//
// Created by Rocky Su on 6/11/20.
//

#include "SimPrototypes.h"

SimPrototypes::SimPrototypes() {
    hamiltonian_prototypes = HamiltonianPrototypes();
    gate_prototypes = GatePrototypes();
    sequence_symbol_alias_map = std::map<std::string, std::string>();
};

SimPrototypes::SimPrototypes(const SimPrototypes &s) {
    //Copy constructer
    hamiltonian_prototypes = * new HamiltonianPrototypes(s.hamiltonian_prototypes);
//    std::cout << s.hamiltonian_prototypes.mw_hamiltonian_prototype_map["PulsedDetuning"] << std::endl;
    gate_prototypes = * new GatePrototypes(s.gate_prototypes);

    sequence_symbol_alias_map = std::map<std::string, std::string>();
    for (const auto& symbol_item : s.sequence_symbol_alias_map) {
        sequence_symbol_alias_map.insert(std::make_pair(symbol_item.first,symbol_item.second));
    }
}

/****************************************************************************************************************/

HamiltonianPrototypes::HamiltonianPrototypes() {
    static_hamiltonian_prototype_map = std::map<hamiltonian_tag_type, Static_Hamiltonian *>();
    mw_hamiltonian_prototype_map = std::map<hamiltonian_tag_type, MW_Hamiltonian *>();
    awg_hamiltonian_prototype_map = std::map<hamiltonian_tag_type, AWG_Hamiltonian *>();
    noise_hamiltonian_prototype_map = std::map<hamiltonian_tag_type, Noise_Hamiltonian *>();
}

HamiltonianPrototypes::HamiltonianPrototypes(const HamiltonianPrototypes &h) {
    //Copy constructer
    static_hamiltonian_prototype_map = std::map<hamiltonian_tag_type, Static_Hamiltonian *>();
    for (const auto& static_h_item : h.static_hamiltonian_prototype_map) {
        static_hamiltonian_prototype_map.insert(std::make_pair(static_h_item.first,static_h_item.second->clone()));
    }

    awg_hamiltonian_prototype_map = std::map<hamiltonian_tag_type, AWG_Hamiltonian *>();
    for (const auto& awg_h_item : h.awg_hamiltonian_prototype_map) {
        awg_hamiltonian_prototype_map.insert(std::make_pair(awg_h_item.first,awg_h_item.second->clone()));
    }

    mw_hamiltonian_prototype_map = std::map<hamiltonian_tag_type, MW_Hamiltonian *>();
    for (const auto& mw_h_item : h.mw_hamiltonian_prototype_map) {
        mw_hamiltonian_prototype_map.insert(std::make_pair(mw_h_item.first,mw_h_item.second->clone()));
    }

    noise_hamiltonian_prototype_map = std::map<hamiltonian_tag_type, Noise_Hamiltonian *>();
    for (const auto& noise_h_item : h.noise_hamiltonian_prototype_map) {
        noise_hamiltonian_prototype_map.insert(std::make_pair(noise_h_item.first,noise_h_item.second->clone()));
    }
}

HamiltonianPrototypes::HamiltonianPrototypes(nlohmann::json h_configs, std::string config_folder) {
    for (auto & h_prototypes_def : h_configs["hamiltonian_prototype_defs"]) {
        std::string tag = h_prototypes_def["tag"];
        std::string hamiltonian_type = h_prototypes_def["type"];
        bool enable = h_prototypes_def["enable"];
        if (enable) {
            if (hamiltonian_type == "static") {
                auto *h_staic = new Static_Hamiltonian(h_prototypes_def, config_folder);
                static_hamiltonian_prototype_map.insert(std::make_pair(tag, h_staic));
            } else if(hamiltonian_type == "mw") {
                auto *mw = new MW_Hamiltonian(h_prototypes_def, config_folder);
                mw_hamiltonian_prototype_map.insert(std::make_pair(tag, mw));
            } else if(hamiltonian_type == "awg") {
                auto *awg_h = new AWG_Hamiltonian(h_prototypes_def, config_folder);
                awg_hamiltonian_prototype_map.insert(std::make_pair(tag, awg_h));
            } else if(hamiltonian_type == "noise") {
                auto *noise = new Noise_Hamiltonian(h_prototypes_def, config_folder);
                noise_hamiltonian_prototype_map.insert(std::make_pair(tag,noise));
            }
        }
    }
}

void HamiltonianPrototypes::update_prototype_with(std::string tag_name, std::string property_name, double value) {
    Hamiltonian * hamiltonian_obj;
    if (awg_hamiltonian_prototype_map.find(tag_name) != awg_hamiltonian_prototype_map.end()){
        hamiltonian_obj = awg_hamiltonian_prototype_map[tag_name];
    }
    else if (mw_hamiltonian_prototype_map.find(tag_name) != mw_hamiltonian_prototype_map.end()){
        hamiltonian_obj = mw_hamiltonian_prototype_map[tag_name];
    }
    else if (static_hamiltonian_prototype_map.find(tag_name) != static_hamiltonian_prototype_map.end()){
        hamiltonian_obj = static_hamiltonian_prototype_map[tag_name];
    }
    else if (noise_hamiltonian_prototype_map.find(tag_name) != noise_hamiltonian_prototype_map.end()){
        hamiltonian_obj = noise_hamiltonian_prototype_map[tag_name];
    }
    rttr::property parametric_prop = rttr::type::get(*hamiltonian_obj).get_property(property_name);
    parametric_prop.set_value(*hamiltonian_obj,value);
}

/****************************************************************************************************************/

GatePrototypes::GatePrototypes() {
    gate_prototype_map = std::map<hamiltonian_tag_type, Gate *>();
}

GatePrototypes::GatePrototypes(const GatePrototypes &g) {
    std::cout << "GatePrototypes: copying"<< std::endl;

    time_step = g.time_step;
    for (const auto& gate_item : g.gate_prototype_map) {
        //std::cout << "GatePrototypes: copy" << gate_item.first << std::endl;
        gate_item.second->step_size = time_step;
        gate_prototype_map.insert(std::make_pair(gate_item.first,gate_item.second->clone()));
    }
}

GatePrototypes::GatePrototypes(nlohmann::json g_configs, std::string config_folder) {

    std::vector<nlohmann::json> gate_defs = g_configs["gate_defs"];
    for (auto & gate_def : gate_defs) {
        Gate *gate_new = new Gate(gate_def,time_step);
        gate_prototype_map.insert(std::make_pair(gate_new->tag, gate_new));
    }

    // Virtual Measurement marker gate. Tagged by "M", 0 pulse_width;
    auto *meas_marker_new = new MeasurementMarker();
    gate_prototype_map.insert(std::make_pair(meas_marker_new->tag,meas_marker_new));
}

void GatePrototypes::update_prototype_with(std::string tag_name, std::string property_name, double value) {
    Gate * gate_obj = gate_prototype_map[tag_name];
    rttr::property parametric_prop = rttr::type::get(*gate_obj).get_property(property_name);
    parametric_prop.set_value(*gate_obj,value);
}

