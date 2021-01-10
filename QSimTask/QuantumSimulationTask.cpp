//
// Created by Rocky Su on 18/9/20.
//

#include "QuantumSimulationTask.h"
#include <iostream>
#include <rttr/registration.h>
#include <rttr/property.h>
#include <rttr/type.h>
#include <sys/stat.h>
#include <dirent.h>
#include "omp.h"

RTTR_REGISTRATION{
    rttr::registration::class_<QSimTask>("QSimTask").constructor<>()
            .property("config_file_folder", &QSimTask::config_file_folder)
            .property("step_size",&QSimTask::step_size);
};

QSimTask::QSimTask() {
    task_time_stamp = get_time_stamp_str();
    char *current_path = getcwd(NULL,0);
    config_file_folder = std::string(current_path).append("/").append(CONFIG_FOLDER_NAME);
    result_output_folder = std::string(current_path).append("/").append(OUTPUT_FOLDER_NAME);
    job_id = 0;
    num_job_group = 1;
}

QSimTask::~QSimTask() {

}

void QSimTask::launch_task() {
    int max_num_of_threads = omp_get_max_threads();
    task_log(std::string("QSimTask: Device has ").append(std::to_string(max_num_of_threads)).append(" threads."),1);
    if(enable_param_parallel_mode) {
        task_log("QSimTask: Task started, sweeping parallelized along parameters",1);
        sweeping_param_parallel();
    } else {
        task_log("QSimTask: Task started, sweeping parallelized along repeat",1);
        param_schedule.process_prameter_vec_with_job_slicing_strategy(job_id,num_job_group,job_slicing_strategy);
        sweeping_repeat_parallel();
    }
}

void QSimTask::sweeping_repeat_parallel() {
    for (int i = 0; i < param_schedule.num_of_params; ++i) {
        std::string result_file_name = std::string(result_exact_path).append("/").append(task_name).append(task_time_stamp).append("_Job#").append(std::to_string(job_id)).append("_meas");
        std::string result_param_str = param_schedule.get_param_string_for_ith_param(i);

        //Reload sweeping param and generate a new prototype set from original one
        SimPrototypes *reloaded_prototype = reload_prototypes_with_sweeping_parameter(i);

        //Generate sequence based on the Gate prototypes and sequence string
        Sequence *seq = new Sequence();
        seq-> load_sequence(step_size, sim_configs["sequence"],reloaded_prototype->sequence_symbol_alias_map,reloaded_prototype->gate_prototype_map);
//        save_gate_switching_map(seq->gate_switching_map,seq->time_vec,result_exact_path,result_param_str);
        int total_num_steps = seq->get_total_num_steps();
        task_log(std::string("Total length of the sequence:").append(std::to_string(total_num_steps)),2);

        //Launch Solver from
        std::vector<arma::cx_cube> rho_multi_temp = std::vector<arma::cx_cube>(rho_inits.size());
        for (auto & rho_t_item : rho_multi_temp) {
            rho_t_item = arma::cx_cube(system_dimension,system_dimension,total_num_steps+1).fill(0);
        }

        //Ctrl time-dep hamiltonian doesn't change per noise iteration.
        auto ctrl_hamiltonian_time_dep = compile_time_dep_ctrl_hamiltonian(reloaded_prototype->ctrl_hamiltonian_prototype_map,reloaded_prototype->gate_prototype_map, *seq);

        std::vector<double > randomstartlist = generate_random_num_list(iterations,3);

        VonNeumannSolver solver_obj = VonNeumannSolver();
        #pragma omp parallel for default(none) shared(reloaded_prototype,randomstartlist,ctrl_hamiltonian_time_dep,system_dimension,total_num_steps,rho_multi_temp) private(solver_obj)
        for (int noise_idx = 0; noise_idx < iterations; ++noise_idx) {
            //Load Noise Hamiltonian
            arma::cx_cube *noise_hamiltonian_time_dep = compile_time_dep_noise_hamiltonian(reloaded_prototype->noise_hamiltonian_prototype_map,noise_idx,randomstartlist,total_num_steps);

            //Load all prepared info to solver
            solver_obj.rho_t_multi = &rho_multi_temp;
            solver_obj.rho0_multi = &rho_inits;
            solver_obj.total_repeat_num = iterations;
            solver_obj.ctrl_hamiltonian_time_dep = ctrl_hamiltonian_time_dep;
            solver_obj.noise_hamiltonian_time_dep = noise_hamiltonian_time_dep;
            solver_obj.calculate_evolution();
//            delete noise_hamiltonian_time_dep;
        }
        task_log("Solver job done!",1);

        MeasurementManager meas_manager = MeasurementManager(observables,rho_inits);
        meas_manager.step_size = step_size;
        meas_manager.measure_from_density_mat_with_time_points(rho_multi_temp,seq->measurement_time_point_vec);
        meas_manager.save_result_to_folder(result_file_name,result_param_str);
        task_log(std::string("Result saved to:").append(result_file_name).append("\n at param:").append(result_param_str),1);

        if (will_record_all_measurement) {
            meas_manager.measure_from_density_mat_with_all_time_points(rho_multi_temp);
            meas_manager.save_result_to_folder(std::string(result_file_name).append("_all"),result_param_str);
        }
    }
}

void QSimTask::sweeping_param_parallel() {
    std::string result_file_name = std::string(result_exact_path).append("/").append(task_name).append(task_time_stamp).append("_Job#").append(std::to_string(job_id)).append("_meas");

    arma::vec noise_index_ends_list = arma::linspace(0,iterations,num_job_group+1);
    int start_pos_for_this_job = noise_index_ends_list.at(job_id);
    int end_pos_for_this_job = noise_index_ends_list.at(job_id+1);
    task_log(std::string("Noise index start:").append(std::to_string(start_pos_for_this_job)).append(" index ends:").append(std::to_string(end_pos_for_this_job)),1);

    #pragma omp parallel for default(none) shared(start_pos_for_this_job,end_pos_for_this_job,result_file_name)
    for (int i = 0; i < param_schedule.num_of_params; ++i) {

        std::string result_param_str = param_schedule.get_param_string_for_ith_param(i);

        //Reload sweeping param and generate a new prototype set from original one
        SimPrototypes *reloaded_prototype = reload_prototypes_with_sweeping_parameter(i);

        //Generate sequence based on the Gate prototypes and sequence string
        Sequence *seq = new Sequence();
        seq-> load_sequence(step_size, sim_configs["sequence"], reloaded_prototype->sequence_symbol_alias_map, reloaded_prototype->gate_prototype_map);
//        save_gate_switching_map(seq->gate_switching_map,seq->time_vec,result_exact_path,result_param_str);
        int total_num_steps = seq->get_total_num_steps();

        //Launch Solver from
        std::vector<arma::cx_cube> rho_multi_temp = std::vector<arma::cx_cube>(rho_inits.size());
        for (auto & rho_t_item : rho_multi_temp) {
            rho_t_item = arma::cx_cube(system_dimension,system_dimension,total_num_steps+1).fill(0);
        }

        //Ctrl time-dep hamiltonian
        auto ctrl_hamiltonian_time_dep = compile_time_dep_ctrl_hamiltonian(reloaded_prototype->ctrl_hamiltonian_prototype_map,reloaded_prototype->gate_prototype_map, *seq);

        std::vector<double > randomstartlist = generate_random_num_list(iterations,3);

        VonNeumannSolver solver_obj = VonNeumannSolver();
        for (int noise_idx = start_pos_for_this_job; noise_idx < end_pos_for_this_job; ++noise_idx) {
            //Load Noise Hamiltonian
            arma::cx_cube *noise_hamiltonian_time_dep = compile_time_dep_noise_hamiltonian(reloaded_prototype->noise_hamiltonian_prototype_map,noise_idx,randomstartlist,total_num_steps);

            //Load all prepared info to solver
            solver_obj.rho_t_multi = &rho_multi_temp;
            solver_obj.rho0_multi = &rho_inits;
            solver_obj.total_repeat_num = iterations;
            solver_obj.ctrl_hamiltonian_time_dep = ctrl_hamiltonian_time_dep;
            solver_obj.noise_hamiltonian_time_dep = noise_hamiltonian_time_dep;
            solver_obj.calculate_evolution();
//            delete noise_hamiltonian_time_dep;
        }
        task_log("Solver job done!",1);

        MeasurementManager meas_manager = MeasurementManager(observables,rho_inits);
        meas_manager.step_size = step_size;
        meas_manager.measure_from_density_mat_with_time_points(rho_multi_temp,seq->measurement_time_point_vec);

        #pragma omp critical
        {
            meas_manager.save_result_to_folder(result_file_name,result_param_str);
        };
        task_log(std::string("Result saved to:").append(result_file_name).append("\n at param:").append(result_param_str),1);

        if (will_record_all_measurement) {
            meas_manager.measure_from_density_mat_with_all_time_points(rho_multi_temp);
            #pragma omp critical
            {
                meas_manager.save_result_to_folder(std::string(result_file_name).append("_all"), result_param_str);
            }
        }
    }
}


void QSimTask::load_sim_configs() {
    task_log("Loading simulation configs",1);

    std::string sim_configfile_path = std::string(config_file_folder).append("/sim_config.json");
    sim_configs = load_config_from_path(sim_configfile_path);

    task_name = sim_configs["task_name"];
    log_level_threshold = sim_configs["log_level"];
    job_slicing_strategy = sim_configs["job_slicing_strategy"];

    param_schedule = ParamScheduler();
//    param_schedule.param_span_exp = sim_configs["sweep_param_span_expression"];
    param_schedule.load_param_info_table_from_json(sim_configs["sweep_param_info"]);
    param_schedule.load_param_from_file(config_file_folder);

    step_size = sim_configs["step_size"];
    iterations = sim_configs["repeat"];
    will_record_all_measurement = sim_configs["record_all_meas"];
    will_record_unitary = sim_configs["record_unitary"];
    enable_param_parallel_mode = sim_configs["enable_param_parallel_mode"];
    system_dimension = sim_configs["system_dim"];

    // Create new folder for result storage. & Backup the config files to the new path.
    DIR *resultDir;
    if ((resultDir=opendir(result_output_folder.c_str()))==NULL) {
        mkdir(result_output_folder.c_str(),0777);
    }
    result_exact_path = std::string(result_output_folder).append("/").append(task_name).append(task_time_stamp);
    DIR *dir;
    if ((dir=opendir(result_exact_path.c_str())) == NULL) {
        mkdir(result_exact_path.c_str(),0777);
        std::string new_config_file_folder = std::string(result_exact_path).append("/config_files");
        mkdir(new_config_file_folder.c_str(),0777);
        std::string copyConfigFileCommand = std::string("cp -v ").append(config_file_folder).append("/* ").append(new_config_file_folder);
        system(copyConfigFileCommand.c_str());
    }

    // Load initial states
    rho_inits = std::vector<symbolic_matrix>();
    int sys_dim = sim_configs["system_dim"];
    std::vector<std::string> rho_init_strs = sim_configs["init_states"];
    for(const auto& rho_init_str : rho_init_strs) {
        symbolic_matrix rho_;
        rho_.load_from_symbol(rho_init_str,config_file_folder);
        rho_.mat /= sys_dim;
        rho_inits.push_back(rho_);
        std::cout << "QSimTask: rho_init:\n" << rho_.mat << std::endl;
    }

    // Load observables
    observables = std::vector<symbolic_matrix>();
    std::vector<std::string> observable_strs = sim_configs["observables"];
    for(const auto& obs_str : observable_strs) {
        symbolic_matrix obs_;
        obs_.load_from_symbol(obs_str,config_file_folder);
        observables.push_back(obs_);
        std::cout << "QSimTask: Observables:\n" << obs_.mat << std::endl;
    }

}

void QSimTask::load_gate_configs() {
    task_log("Loading Gate prototypes",1);
    std::string gate_configfile_path = std::string(config_file_folder).append("/gate_config.json");
    gate_configs = load_config_from_path(gate_configfile_path);
    auto gate_prototype_map = std::map<gate_tag_type,Gate *>();

    std::vector<nlohmann::json> gate_defs = gate_configs["gate_defs"];
    for (auto & gate_def : gate_defs) {
        Gate *gate_new = new Gate(gate_def,step_size);
        gate_prototype_map.insert(std::make_pair(gate_new->tag, gate_new));
    }

    // Virtual Measurement marker gate. Tagged by "M", 0 pulse_width;
    auto *meas_marker_new = new MeasurementMarker();
    gate_prototype_map.insert(std::make_pair(meas_marker_new->tag,meas_marker_new));

    simulation_prototypes.gate_prototype_map = gate_prototype_map;
}

void QSimTask::load_hamiltonian_configs() {
    task_log("Loading Hamiltonian prototypes",1);

    std::string hamiltonian_configfile_path = std::string(config_file_folder).append("/hamiltonian_config.json");
    hamiltonian_configs = load_config_from_path(hamiltonian_configfile_path);
    auto ctrl_hamiltonian_prototype_map = std::map<hamiltonian_tag_type,Hamiltonian *>();
    auto noise_hamiltonian_prototype_map = std::map<hamiltonian_tag_type,Noise_Hamiltonian *>();

    std::vector<nlohmann::json> h_prototypes_defs = hamiltonian_configs["hamiltonian_prototype_defs"];
    //TODO: Implement by reflection(RTTR) will be a more elegant solution!
    for (auto & h_prototypes_def : h_prototypes_defs) {
        std::string tag = h_prototypes_def["tag"];
        std::string hamiltonian_type = h_prototypes_def["type"];
        bool enable = h_prototypes_def["enable"];
        if (enable) {
            task_log(std::string("Loading Hamiltonian tag:").append(tag),2);
            if (hamiltonian_type == "static") {
                auto *h_staic = new Static_Hamiltonian(h_prototypes_def, config_file_folder);
                ctrl_hamiltonian_prototype_map.insert(std::make_pair(tag, h_staic));
            } else if(hamiltonian_type == "mw") {
                auto *mw = new MW_Hamiltonian(h_prototypes_def, config_file_folder);
                ctrl_hamiltonian_prototype_map.insert(std::make_pair(tag,mw));
            } else if(hamiltonian_type == "awg") {
                auto *awg_h = new AWG_Hamiltonian(h_prototypes_def, config_file_folder);
                ctrl_hamiltonian_prototype_map.insert(std::make_pair(tag,awg_h));
            } else if(hamiltonian_type == "noise") {
                auto *noise = new Noise_Hamiltonian(h_prototypes_def, config_file_folder);
                noise_hamiltonian_prototype_map.insert(std::make_pair(tag,noise));
            }
        }
    }

    for (auto const &ctrl_H_item:ctrl_hamiltonian_prototype_map) {
        ctrl_H_item.second->step_size = step_size;
    }
    for (auto const &noise_H_item:noise_hamiltonian_prototype_map) {
        noise_H_item.second->step_size = step_size;
    }

    simulation_prototypes.ctrl_hamiltonian_prototype_map = ctrl_hamiltonian_prototype_map;
    simulation_prototypes.noise_hamiltonian_prototype_map = noise_hamiltonian_prototype_map;
}

SimPrototypes* QSimTask::reload_prototypes_with_sweeping_parameter(int index) {
    SimPrototypes *reloaded_prototypes = new SimPrototypes(simulation_prototypes);

    for (int i = 0; i < param_schedule.param_info_table.size(); ++i) {
        auto type_name = param_schedule.param_info_table.at(i).at("class");
        auto tag = param_schedule.param_info_table.at(i).at("tag");
        auto property_name = param_schedule.param_info_table.at(i).at("prop");

        task_log(std::string("Reloading:").append(type_name).append(":").append(tag).append(" on field:").append(property_name),1);

        if (type_name == "Gate") {
            auto val_name = param_schedule.param_info_table.at(i).at("val_file");
            auto param_val = param_schedule.param_val_vec_map.at(val_name).at(index);
            Gate * gate_obj = reloaded_prototypes->gate_prototype_map[tag];
            rttr::property parametric_prop = rttr::type::get(*gate_obj).get_property(property_name);
            parametric_prop.set_value(*gate_obj,param_val);
            std::cout << "QSimTask: Parameter " << ", with val=" << std::to_string(param_val) << " is reloaded" << std::endl;
        } else if (type_name == "Hamiltonian") {
            auto val_name = param_schedule.param_info_table.at(i).at("val_file");
            auto param_val = param_schedule.param_val_vec_map.at(val_name).at(index);
            Hamiltonian * hamiltonian_obj;
            if (reloaded_prototypes->ctrl_hamiltonian_prototype_map.find(tag) != reloaded_prototypes->ctrl_hamiltonian_prototype_map.end()){
                hamiltonian_obj = reloaded_prototypes->ctrl_hamiltonian_prototype_map[tag];
            } else if (reloaded_prototypes->noise_hamiltonian_prototype_map.find(tag) != reloaded_prototypes->noise_hamiltonian_prototype_map.end()) {
                hamiltonian_obj = reloaded_prototypes->noise_hamiltonian_prototype_map[tag];
            }
            rttr::property parametric_prop = rttr::type::get(*hamiltonian_obj).get_property(property_name);
            parametric_prop.set_value(*hamiltonian_obj,param_val);
            std::cout << "QSimTask: Parameter " << ", with val=" << std::to_string(param_val) << " is reloaded" << std::endl;
        } else if (type_name == "Sequence") {
            auto string_file_name = param_schedule.param_info_table.at(i).at("string_file");
            auto param_string = param_schedule.param_string_vec_map.at(string_file_name).at(index);
            reloaded_prototypes->sequence_symbol_alias_map[tag] = param_string;
            std::cout << "QSimTask: Parameter " << ", with string=" << string_file_name << " is reloaded" << std::endl;
        }
    }

    return reloaded_prototypes;
}

/************************************PRIVATE FUNCTIONS******************************************************************/

nlohmann::json QSimTask::load_config_from_path(const std::string& path) {
    nlohmann::json config_json;
    std::ifstream file(path);
    if (file) {
        file >> config_json;
        file.close();
    }
    return config_json;
}

void QSimTask::task_log(std::string message, int log_level) {
    if (log_level < log_level_threshold) {
        std::string log_msg = std::string("QSimTask:").append(message);
        std::cout << log_msg << std::endl;
    }
}

arma::cx_cube*
QSimTask::compile_time_dep_ctrl_hamiltonian(std::map<hamiltonian_tag_type, Hamiltonian *> hamiltonian_prototype_map,
                                            std::map<gate_tag_type, Gate *> gate_map, Sequence seq) {

    for (const auto& gate_item : gate_map) {
        auto gate_proto_tag = gate_item.first;
        auto gate_proto_obj = gate_item.second;

        for (const auto& binded_hamiltonian_tag : gate_proto_obj->hamiltonian_tags_list) {
            hamiltonian_prototype_map[binded_hamiltonian_tag]->add_signal(seq.gate_switching_map[gate_proto_tag]);
        }
    }

    auto * ctrl_hamiltonian_time_dep = new arma::cx_cube(system_dimension,system_dimension,seq.get_total_num_steps());
    ctrl_hamiltonian_time_dep->fill(0);
    for (const auto& hamiltonian_item : hamiltonian_prototype_map) {
        hamiltonian_item.second->num_of_steps = seq.get_total_num_steps();
        hamiltonian_item.second->step_size = step_size;
        hamiltonian_item.second->load_waveform();
        hamiltonian_item.second->fetch_H(ctrl_hamiltonian_time_dep);
    }
    task_log(" Control Hamiltonians preloaded",2);

    return ctrl_hamiltonian_time_dep;
}

arma::cx_cube* QSimTask::compile_time_dep_noise_hamiltonian (
        std::map<hamiltonian_tag_type, Noise_Hamiltonian *> hamiltonian_prototype_map,
        int noise_idx,
        std::vector<double> random_start_pos_factor,
        int total_num_steps) {

    arma::cx_cube *noise_hamiltonian_time_dep = new arma::cx_cube(system_dimension,system_dimension,total_num_steps);
    noise_hamiltonian_time_dep->fill(0);
    for (const auto& noise_hamiltonian_item : hamiltonian_prototype_map) {
        auto * noise_h_temp = new Noise_Hamiltonian(*noise_hamiltonian_item.second);
        noise_h_temp->num_of_steps = total_num_steps;
        noise_h_temp->step_size = step_size;
        noise_h_temp->randomStartPosFactor = random_start_pos_factor.at(noise_idx);
        noise_h_temp->load_ext_waveform(noise_idx);
        noise_h_temp->fetch_H(noise_hamiltonian_time_dep);
    }

    return noise_hamiltonian_time_dep;
}
