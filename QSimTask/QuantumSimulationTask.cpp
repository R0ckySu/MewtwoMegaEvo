//
// Created by Rocky Su on 18/9/20.
//

#include "QuantumSimulationTask.h"
#include "QSimCoreLib/Utils.h"
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

}

QSimTask::~QSimTask() {

}

void QSimTask::load_sim_configs() {
    std::string sim_configfile_path = std::string(config_file_folder).append("/sim_config.json");
    sim_configs = load_config_from_path(sim_configfile_path);

    param_vec.load(sim_configs["sweep_val_path"],arma::csv_ascii);
    step_size = sim_configs["step_size"];
    iterations = sim_configs["iterations"];

    rho_inits = std::vector<arma::cx_mat>();
    int sys_dim = sim_configs["system_dim"];
    std::vector<std::string> rho_init_strs = sim_configs["init_states"];
    for(const auto& rho_init_str : rho_init_strs) {
        arma::cx_mat rho_ = qmt::spinorDecoder(rho_init_str)/sys_dim;
        rho_inits.push_back(rho_);
        std::cout << "QSimTask: rho_init:\n" << rho_ << std::endl;
    }
}

void QSimTask::load_gate_configs() {
    std::string gate_configfile_path = std::string(config_file_folder).append("/gate_config.json");
    gate_configs = load_config_from_path(gate_configfile_path);
    gate_prototype_map = std::map<gate_tag_type,Gate *>();

    std::vector<nlohmann::json> gate_defs = gate_configs["gate_defs"];
    for (auto & gate_def : gate_defs) {
        Gate *gate_new = new Gate(gate_def);
        gate_new->hamiltonian_obj_map_ptr = &ctrl_hamiltonian_prototype_map;
        gate_prototype_map.insert(std::make_pair(gate_new->tag, gate_new));
    }
}

void QSimTask::load_hamiltonian_configs() {
    std::string hamiltonian_configfile_path = std::string(config_file_folder).append("/hamiltonian_config.json");
    hamiltonian_configs = load_config_from_path(hamiltonian_configfile_path);
    ctrl_hamiltonian_prototype_map = std::map<hamiltonian_tag_type,Hamiltonian *>();
    noise_hamiltonian_prototype_map = std::map<hamiltonian_tag_type,Noise_Hamiltonian *>();

    std::vector<nlohmann::json> h_prototypes_defs = hamiltonian_configs["hamiltonian_prototype_defs"];
    //TODO: Implement by reflection will be a more elegant solution!
    for (auto & h_prototypes_def : h_prototypes_defs) {
        std::string tag = h_prototypes_def["tag"];
        std::string hamiltonian_type = h_prototypes_def["type"];
        if (hamiltonian_type == "static") {
            auto *h_staic = new Static_Hamiltonian(h_prototypes_def);
            ctrl_hamiltonian_prototype_map.insert(std::make_pair(tag, h_staic));
        } else if(hamiltonian_type == "mw") {
            auto *mw = new MW_Hamiltonian(h_prototypes_def);
            ctrl_hamiltonian_prototype_map.insert(std::make_pair(tag,mw));
        } else if(hamiltonian_type == "awg") {

        } else if(hamiltonian_type == "noise") {
            auto *noise = new Noise_Hamiltonian(h_prototypes_def);
            noise_hamiltonian_prototype_map.insert(std::make_pair(tag,noise));
        }
    }
}

Sequence* QSimTask::load_sequence() {
    Sequence *seq = new Sequence();
    seq->step_size = step_size;

    auto gate_info_pair_vec = symbolic_sequence_decoder(sim_configs["sequence"]);
    for (const auto& info_pair : gate_info_pair_vec) {
        std::string gate_tag = info_pair.first;
        std::string gate_param_str = info_pair.second;
        Gate gate_new = *gate_prototype_map[gate_tag];
        gate_new.decode_param_str(gate_param_str);
        seq->append_gate(gate_new);
    }

    seq->generate_switching_sig();

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
    return seq;
}

VonNeumannSolver* QSimTask::launch_solver(int total_num_steps,int matrix_dim) {

    std::vector<arma::cx_cube> rho_multi_temp = std::vector<arma::cx_cube>(rho_inits.size());
    for (auto & rho_t_item : rho_multi_temp) {
        rho_t_item = arma::cx_cube(matrix_dim,matrix_dim,total_num_steps+1);
    }

    std::cout << "QSimTask: rho initialised for solver" << std::endl;

    auto * ctrl_hamiltonian_time_dep = new arma::cx_cube(matrix_dim,matrix_dim,total_num_steps);
    ctrl_hamiltonian_time_dep->fill(0);
    for (const auto& hamiltonian_item : ctrl_hamiltonian_prototype_map) {
        hamiltonian_item.second->num_of_steps = total_num_steps;
        hamiltonian_item.second->step_size = step_size;
        hamiltonian_item.second->load_waveform();
        hamiltonian_item.second->fetch_H(ctrl_hamiltonian_time_dep);
    }
    std::cout << "QSimTask: Control Hamiltonians linked to solver" << std::endl;

    arma::cx_cube * noise_hamiltonian_time_dep = new arma::cx_cube(matrix_dim,matrix_dim,total_num_steps);

//    printf("Max num Thread: %d \n",omp_get_max_threads());

    VonNeumannSolver solver_obj = * new VonNeumannSolver();

    solver_obj.noise_hamiltonian_time_dep = noise_hamiltonian_time_dep;

    #pragma omp parallel for default(none) shared(ctrl_hamiltonian_time_dep,matrix_dim,total_num_steps,rho_multi_temp) private(solver_obj)
    for (int i = 0; i < iterations; ++i) {
        solver_obj.rho_t_multi = &rho_multi_temp;
        solver_obj.rho0_multi = &rho_inits;
        solver_obj.ctrl_hamiltonian_time_dep = ctrl_hamiltonian_time_dep;

        arma::cx_cube * noise_hamiltonian_time_dep_per_iter = new arma::cx_cube(matrix_dim,matrix_dim,total_num_steps);
        for (const auto& noise_hamiltonian_item : noise_hamiltonian_prototype_map) {
            auto * noise_h_temp = new Noise_Hamiltonian(*noise_hamiltonian_item.second);
            noise_h_temp->num_of_steps = total_num_steps;
            noise_h_temp->step_size = step_size;
            noise_h_temp->load_ext_waveform(i);
            noise_h_temp->fetch_H(noise_hamiltonian_time_dep_per_iter);
        }
        solver_obj.noise_hamiltonian_time_dep = noise_hamiltonian_time_dep_per_iter;
//        std::cout << "QSimTask: Noise Hamiltonians linked to solver, index=" << std::to_string(i) << " at thread:" << std::to_string(omp_get_num_threads()) << std::endl;

        solver_obj.calculate_evolution();
    }

//    solver_obj.rho_t_multi->at(1).save(std::string(config_file_folder).append("/rhotest0"),arma::hdf5_binary);

    return &solver_obj;
}

void QSimTask::measument_solver() {

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


