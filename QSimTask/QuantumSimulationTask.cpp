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

sim_prototypes::sim_prototypes() {
    ctrl_hamiltonian_prototype_map = std::map<hamiltonian_tag_type, Hamiltonian *>();
    noise_hamiltonian_prototype_map = std::map<hamiltonian_tag_type, Noise_Hamiltonian *>();
    gate_prototype_map = std::map<hamiltonian_tag_type, Gate *>();
};

sim_prototypes::sim_prototypes(const sim_prototypes &s) {
    //Copy constructer
    ctrl_hamiltonian_prototype_map = std::map<hamiltonian_tag_type, Hamiltonian *>();
    for (const auto& ctrl_h_item : s.ctrl_hamiltonian_prototype_map) {
        ctrl_hamiltonian_prototype_map.insert(std::make_pair(ctrl_h_item.first,ctrl_h_item.second->clone()));
    }

    noise_hamiltonian_prototype_map = std::map<hamiltonian_tag_type, Noise_Hamiltonian *>();
    for (const auto& noise_h_item : s.noise_hamiltonian_prototype_map) {
        noise_hamiltonian_prototype_map.insert(std::make_pair(noise_h_item.first,noise_h_item.second->clone()));
    }

    gate_prototype_map = std::map<hamiltonian_tag_type, Gate *>();
    for (const auto& gate_item : s.gate_prototype_map) {
        gate_prototype_map.insert(std::make_pair(gate_item.first,gate_item.second->clone()));
    }
};

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

void QSimTask::sweeping_task() {
    task_log("QSimTask: Parametric Sweeping started",1);
    //TODO: Configurable parallelization by CMake predefined params (Build time config)
    for (int i = 0; i < param_vec.size(); ++i) {
        std::string result_file_name = std::string(result_exact_path).append("/").append(task_name).append(task_time_stamp).append("_Job#").append(std::to_string(job_id)).append("_meas");
        std::string result_param_str = double_to_fixprecision_str(param_vec.at(i),4);

        //Reload sweeping param and generate a new prototype set from original one
        sim_prototypes *reloaded_prototype = reload_prototypes_with_sweeping_parameter(i);

        //Generate sequence based on the Gate prototypes and sequence string
        Sequence *seq = new Sequence();
        seq-> load_sequence(step_size, sim_configs["sequence"],reloaded_prototype->gate_prototype_map);

        //Launch Solver from
        std::vector<arma::cx_cube> rho_multi_result = launch_solver(seq, reloaded_prototype);

        MeasurementManager meas_manager = MeasurementManager(observables,rho_inits);
        meas_manager.step_size = step_size;
        meas_manager.measure_from_density_mat_with_time_points(rho_multi_result,seq->measurement_time_point_vec);
        meas_manager.save_result_to_folder(result_file_name,result_param_str);
        task_log(std::string("Result saved to:").append(result_file_name).append("\n at param:").append(result_param_str),1);

        if (will_record_all_measurement) {
            meas_manager.measure_from_density_mat_with_all_time_points(rho_multi_result);
            meas_manager.save_result_to_folder(std::string(result_file_name).append("_all"),result_param_str);
        }

        delete seq;
    }
}


void QSimTask::load_sim_configs() {
    task_log("Loading simulation configs",1);

    std::string sim_configfile_path = std::string(config_file_folder).append("/sim_config.json");
    sim_configs = load_config_from_path(sim_configfile_path);

    task_name = sim_configs["task_name"];
    log_level_threshold = sim_configs["log_level"];
    job_slicing_strategy = sim_configs["job_slicing_strategy"];
    std::string sweep_val_file_name = sim_configs["sweep_val_file"];
    param_vec.load(std::string(config_file_folder).append("/").append(sweep_val_file_name),arma::csv_ascii);
    step_size = sim_configs["step_size"];
    iterations = sim_configs["iterations"];
    will_record_all_measurement = sim_configs["record_all_meas"];
    will_record_unitary = sim_configs["record_unitary"];
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

    process_prameter_vec_with_job_slicing_strategy();
}

void QSimTask::load_gate_configs() {
    task_log("Loading Gate prototypes",1);
    std::string gate_configfile_path = std::string(config_file_folder).append("/gate_config.json");
    gate_configs = load_config_from_path(gate_configfile_path);
    auto gate_prototype_map = std::map<gate_tag_type,Gate *>();

    std::vector<nlohmann::json> gate_defs = gate_configs["gate_defs"];
    for (auto & gate_def : gate_defs) {
        Gate *gate_new = new Gate(gate_def,step_size);
        gate_new->step_size = step_size;
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

std::vector<arma::cx_cube> QSimTask::launch_solver(Sequence *seq, sim_prototypes *prototypes) {
    int total_num_steps = seq->get_total_num_steps();
    int matrix_dim = rho_inits.begin()->mat.n_cols;

    //Ctrl time-dep hamiltonian doesn't change per noise iteration.
    auto ctrl_hamiltonian_time_dep = compile_time_dep_ctrl_hamiltonian(prototypes->ctrl_hamiltonian_prototype_map,prototypes->gate_prototype_map, *seq);

    std::vector<arma::cx_cube> rho_multi_temp = std::vector<arma::cx_cube>(rho_inits.size());
    for (auto & rho_t_item : rho_multi_temp) {
        rho_t_item = arma::cx_cube(matrix_dim,matrix_dim,total_num_steps+1).fill(0);
    }

    task_log("Initial state initialised for solver",2);
    VonNeumannSolver solver_obj = VonNeumannSolver();

    std::vector<double > randomstartlist = generate_random_num_list(iterations,3);

    #pragma omp parallel for default(none) shared(prototypes,randomstartlist,ctrl_hamiltonian_time_dep,matrix_dim,total_num_steps,rho_multi_temp) private(solver_obj)
    for (int i = 0; i < iterations; ++i) {
        //Load Noise Hamiltonian
        arma::cx_cube *noise_hamiltonian_time_dep = compile_time_dep_noise_hamiltonian(prototypes->noise_hamiltonian_prototype_map,i,randomstartlist,total_num_steps);

        solver_obj.rho_t_multi = &rho_multi_temp;
        solver_obj.rho0_multi = &rho_inits;
        solver_obj.total_repeat_num = iterations;
        solver_obj.ctrl_hamiltonian_time_dep = ctrl_hamiltonian_time_dep;
        solver_obj.noise_hamiltonian_time_dep = noise_hamiltonian_time_dep;
        solver_obj.calculate_evolution();
    }

//    std::cout << "Solver result length:" << rho_multi_temp.at(0).n_slices << std::endl;
    task_log("Solver job done!",1);
    return rho_multi_temp;
}

void QSimTask::measument_solver() {

    task_log("Finished measurements!",1);
}

sim_prototypes* QSimTask::reload_prototypes_with_sweeping_parameter(int index) {
    sim_prototypes *reloaded_prototypes = new sim_prototypes(simulation_prototypes);

    std::vector<std::string> param_info = str_split(sim_configs["sweep_param_name"],':');
    auto type_name = param_info[0];
    auto tag = param_info[1];
    auto property_name = param_info[2];
    auto param_val = param_vec.at(index);
    task_log(std::string("Reloading:").append(type_name).append(":").append(tag).append(" on field:").append(property_name),1);

    if (type_name == "Gate") {
        Gate * gate_obj = reloaded_prototypes->gate_prototype_map[tag];
        rttr::property parametric_prop = rttr::type::get(*gate_obj).get_property(property_name);
        parametric_prop.set_value(*gate_obj,param_val);
    } else if (type_name == "Hamiltonian") {
        Hamiltonian * hamiltonian_obj;
        if (reloaded_prototypes->ctrl_hamiltonian_prototype_map.find(tag) != reloaded_prototypes->ctrl_hamiltonian_prototype_map.end()){
            hamiltonian_obj = reloaded_prototypes->ctrl_hamiltonian_prototype_map[tag];
        } else if (reloaded_prototypes->noise_hamiltonian_prototype_map.find(tag) != reloaded_prototypes->noise_hamiltonian_prototype_map.end()) {
            hamiltonian_obj = reloaded_prototypes->noise_hamiltonian_prototype_map[tag];
        }
        rttr::property parametric_prop = rttr::type::get(*hamiltonian_obj).get_property(property_name);
        parametric_prop.set_value(*hamiltonian_obj,param_val);
    }

    std::cout << "QSimTask: Parameter " << sim_configs["sweep_param_name"] << ", with val=" << std::to_string(param_val) << " is reloaded" << std::endl;
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

void QSimTask::process_prameter_vec_with_job_slicing_strategy() {
    int num_of_params = param_vec.size();
    if ((num_job_group > num_of_params) || (job_id>num_job_group)) {
        num_job_group = 1;
        job_id = 0;
        task_log(std::string("Wrong slicing parameters!"),1);
        return;
    }

    if (job_slicing_strategy == JOB_SLICING_LINSPACE) {
        arma::vec index_ends_list = arma::linspace(0,num_of_params,num_job_group+1);
        int start_pos_for_this_job = index_ends_list.at(job_id);
        int end_pos_for_this_job = index_ends_list.at(job_id+1);
        param_vec = param_vec.subvec(start_pos_for_this_job,end_pos_for_this_job-1);
    } else if (job_slicing_strategy == JOB_SLICING_LOGSPACE) {
        arma::vec index_ends_list = arma::round(arma::logspace(0,log10(num_of_params),num_job_group));
        index_ends_list.insert_rows(0,1);
        for (int i = 1; i < index_ends_list.size()-1; ++i) {
            if (index_ends_list.at(i) == index_ends_list.at(i-1)) {
                index_ends_list.at(i) = index_ends_list.at(i)+1;
            }
        }
        int start_pos_for_this_job = index_ends_list.at(job_id);
        int end_pos_for_this_job = index_ends_list.at(job_id+1);
        param_vec = param_vec.subvec(start_pos_for_this_job,end_pos_for_this_job-1);
    } else if (job_slicing_strategy == JOB_SLICING_INVLOGSPACE) {
        arma::vec index_ends_list = arma::round(arma::logspace(0,log10(num_of_params),num_job_group));
        index_ends_list.insert_rows(0,1);
        for (int i = 1; i < index_ends_list.size()-1; ++i) {
            if (index_ends_list.at(i) == index_ends_list.at(i-1)) {
                index_ends_list.at(i) = index_ends_list.at(i)+1;
            }
        }
        index_ends_list = num_of_params - index_ends_list;
        int start_pos_for_this_job = index_ends_list.at(job_id);
        int end_pos_for_this_job = index_ends_list.at(job_id+1);
        param_vec = param_vec.subvec(start_pos_for_this_job,end_pos_for_this_job-1);
    }
    std::stringstream param_str;
    param_str << param_vec;
    task_log(std::string("Job sliced, param vec:\n").append(param_str.str()),1);
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