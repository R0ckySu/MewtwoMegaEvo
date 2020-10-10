//
// Created by Rocky Su on 18/9/20.
//

#include "QuantumSimulationTask.h"
#include <iostream>
#include <rttr/registration.h>
#include <rttr/property.h>
#include <rttr/type.h>

RTTR_REGISTRATION{
    rttr::registration::class_<QSimTask>("QSimTask").constructor<>()
            .property("config_file_folder", &QSimTask::config_file_folder)
            .property("step_size",&QSimTask::step_size);
};

QSimTask::QSimTask() {
    task_time_stamp = get_time_stamp_str();
}

QSimTask::~QSimTask() {

}

void QSimTask::sweeping_task() {
    std::cout << "QSimTask: Parametric Sweeping started\n" << std::endl;
    //TODO: Grouped parameters for job slicing.
    //TODO: Configurable parallelization by CMake predefined params (Build time config)
    for (int i = 0; i < param_vec.size(); ++i) {
        reload_with_sweeping_parameter(i);
        Sequence* seq = load_sequence();
        MeasurementManager meas_manager = MeasurementManager(observables,rho_inits);
        meas_manager.step_size = step_size;
        auto rho_t_multi_result = launch_solver(seq->get_total_num_steps(),sim_configs["system_dim"]);

        std::string result_file_name = std::string(result_output_folder).append("/").append(task_name).append(task_time_stamp);
        std::string result_param_str = double_to_fixprecision_str(param_vec.at(i),4);

        meas_manager.measure_from_density_mat_with_time_points(rho_t_multi_result,seq->measurement_time_point_vec);
        meas_manager.save_result_to_folder(result_file_name,result_param_str);
        task_log(std::string("Result saved to:").append(result_file_name).append("\n at param:").append(result_param_str),1);

        if (will_record_all_measurement) {
            meas_manager.measure_from_density_mat_with_all_time_points(rho_t_multi_result);
            meas_manager.save_result_to_folder(std::string(result_file_name).append("_all"),result_param_str);
        }
    }
}


void QSimTask::load_sim_configs() {
    task_log("Loading simulation configs",1);

    std::string sim_configfile_path = std::string(config_file_folder).append("/sim_config.json");
    sim_configs = load_config_from_path(sim_configfile_path);

    task_name = sim_configs["task_name"];
    log_level_threshold = sim_configs["log_level"];
    param_vec.load(sim_configs["sweep_val_path"],arma::csv_ascii);
    step_size = sim_configs["step_size"];
    iterations = sim_configs["iterations"];
    will_record_all_measurement = sim_configs["record_all_meas"];
    will_record_unitary = sim_configs["record_unitary"];

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
    gate_prototype_map = std::map<gate_tag_type,Gate *>();

    std::vector<nlohmann::json> gate_defs = gate_configs["gate_defs"];
    for (auto & gate_def : gate_defs) {
        Gate *gate_new = new Gate(gate_def);
        gate_prototype_map.insert(std::make_pair(gate_new->tag, gate_new));
    }

    // Virtual Measurement marker gate. Tagged by "M", 0 pulse_width;
    auto *meas_marker_new = new MeasurementMarker();
    gate_prototype_map.insert(std::make_pair(meas_marker_new->tag,meas_marker_new));
}

void QSimTask::load_hamiltonian_configs() {
    task_log("Loading Hamiltonian prototypes",1);

    std::string hamiltonian_configfile_path = std::string(config_file_folder).append("/hamiltonian_config.json");
    hamiltonian_configs = load_config_from_path(hamiltonian_configfile_path);
    ctrl_hamiltonian_prototype_map = std::map<hamiltonian_tag_type,Hamiltonian *>();
    noise_hamiltonian_prototype_map = std::map<hamiltonian_tag_type,Noise_Hamiltonian *>();

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
}

Sequence* QSimTask::load_sequence() {
    Sequence *seq = new Sequence();
    seq->step_size = step_size;

    // Decode symbolic sequence
    auto gate_info_pair_vec = symbolic_sequence_str_parser(sim_configs["sequence"]);
    for (const auto& info_pair : gate_info_pair_vec) {
        std::string gate_tag = info_pair.first;
        std::string gate_param_str = info_pair.second;
        task_log(std::string("Sequence add Gate:").append(gate_tag).append(" with param:").append(gate_param_str),3);
        if ( *gate_prototype_map.find(gate_tag) != *gate_prototype_map.end() ) {
            // Gate found
            Gate * gate_new = new Gate(*gate_prototype_map[gate_tag]);
            if (!gate_param_str.empty()) {gate_new->decode_param_str(gate_param_str);}
            seq->append_gate(*gate_new);
        }
    }

    // Generate swiching signal after gates loaded to sequeces
    task_log("Sequence is generating switching signal",1);
    seq->generate_switching_sig();

    // Print measurement time points;
    arma::vec time_points = arma::vec(seq->measurement_time_point_vec);
    std::stringstream vec_str;
    vec_str << time_points;
    task_log(std::string("Marker measurement time points are:\n").append(vec_str.str()),2);

    for (auto ctrl_h_pair : ctrl_hamiltonian_prototype_map){
        ctrl_h_pair.second->clean_up_on_reload();
    }
    for (auto noise_h_pair : noise_hamiltonian_prototype_map){
        noise_h_pair.second->clean_up_on_reload();
    }

    for (const auto& gate_item : gate_prototype_map) {
        auto gate_proto_tag = gate_item.first;
        auto gate_proto_obj = gate_item.second;

        for (const auto& binded_hamiltonian_tag : gate_proto_obj->hamiltonian_tags_list) {
            if(ctrl_hamiltonian_prototype_map[binded_hamiltonian_tag]->switching_signal.empty()) {
                ctrl_hamiltonian_prototype_map[binded_hamiltonian_tag]->switching_signal = seq->gate_switching_map[gate_proto_tag];
            } else {
                ctrl_hamiltonian_prototype_map[binded_hamiltonian_tag]->switching_signal += seq->gate_switching_map[gate_proto_tag];
            }
        }
    }
    task_log("Sequence loaded!",1);
    return seq;
}

std::vector<arma::cx_cube> QSimTask::launch_solver(int total_num_steps,int matrix_dim) {

    std::vector<arma::cx_cube> rho_multi_temp = std::vector<arma::cx_cube>(rho_inits.size());
    for (auto & rho_t_item : rho_multi_temp) {
        rho_t_item = arma::cx_cube(matrix_dim,matrix_dim,total_num_steps+1);
    }

    task_log("Initial state initialised for solver",2);

    auto * ctrl_hamiltonian_time_dep = new arma::cx_cube(matrix_dim,matrix_dim,total_num_steps);
    ctrl_hamiltonian_time_dep->fill(0);
    for (const auto& hamiltonian_item : ctrl_hamiltonian_prototype_map) {
        hamiltonian_item.second->num_of_steps = total_num_steps;
        hamiltonian_item.second->step_size = step_size;
        hamiltonian_item.second->load_waveform();
        hamiltonian_item.second->fetch_H(ctrl_hamiltonian_time_dep);
    }
    task_log(" Control Hamiltonians linked to solver",2);

    VonNeumannSolver solver_obj = * new VonNeumannSolver();

    //TODO: Configurable parallelization by CMake predefined params (Build time config)
    #pragma omp parallel for default(none) shared(ctrl_hamiltonian_time_dep,matrix_dim,total_num_steps,rho_multi_temp) private(solver_obj)
    for (int i = 0; i < iterations; ++i) {
        solver_obj.rho_t_multi = &rho_multi_temp;
        solver_obj.rho0_multi = &rho_inits;
        solver_obj.ctrl_hamiltonian_time_dep = ctrl_hamiltonian_time_dep;

        //Load Noise Hamiltonian
        arma::cx_cube * noise_hamiltonian_time_dep_per_iter = new arma::cx_cube(matrix_dim,matrix_dim,total_num_steps);
        for (const auto& noise_hamiltonian_item : noise_hamiltonian_prototype_map) {
            auto * noise_h_temp = new Noise_Hamiltonian(*noise_hamiltonian_item.second);
            noise_h_temp->num_of_steps = total_num_steps;
            noise_h_temp->step_size = step_size;
            noise_h_temp->load_ext_waveform(i);
            noise_h_temp->fetch_H(noise_hamiltonian_time_dep_per_iter);
        }
        solver_obj.noise_hamiltonian_time_dep = noise_hamiltonian_time_dep_per_iter;

        //Start calculating time evolution
        solver_obj.calculate_evolution();
    }

    task_log("Solver job done!",1);
    return rho_multi_temp;
}

void QSimTask::measument_solver() {

    task_log("Finished measurements!",1);
}

void QSimTask::reload_with_sweeping_parameter(int index) {
    std::vector<std::string> param_info = str_split(sim_configs["sweep_param_name"],':');
    auto type_name = param_info[0];
    auto tag = param_info[1];
    auto property_name = param_info[2];
    auto param_val = param_vec.at(index);

    if (type_name == "Gate") {
        Gate * gate_obj = gate_prototype_map[tag];
        rttr::property parametric_prop = rttr::type::get(*gate_obj).get_property(property_name);
        parametric_prop.set_value(*gate_obj,param_val);
    } else if (type_name == "Hamiltonian") {
        Hamiltonian * hamiltonian_obj;
        if (ctrl_hamiltonian_prototype_map.find(tag) == ctrl_hamiltonian_prototype_map.end()){
            hamiltonian_obj = ctrl_hamiltonian_prototype_map[tag];
        } else if (noise_hamiltonian_prototype_map.find(tag) == noise_hamiltonian_prototype_map.end()) {
            hamiltonian_obj = noise_hamiltonian_prototype_map[tag];
        }
        hamiltonian_obj->clean_up_on_reload();
        rttr::property parametric_prop = rttr::type::get(*hamiltonian_obj).get_property(property_name);
        parametric_prop.set_value(*hamiltonian_obj,param_val);
    } else if (type_name == "Sim") {
        rttr::property parametric_prop = rttr::type::get(*this).get_property(property_name);
        parametric_prop.set_value(*this,param_val);
    }

    std::cout << "QSimTask: Parameter " << sim_configs["sweep_param_name"] << ", with val=" << std::to_string(param_val) << " is reloaded" << std::endl;
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

