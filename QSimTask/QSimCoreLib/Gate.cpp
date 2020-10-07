//
// Created by Rocky Su on 17/9/20.
//

#include "Gate.h"
#include "Utils.h"
#include <rttr/registration.h>

Gate::Gate() {
    tag = "";
    hamiltonian_tags_list = std::vector<std::string>();
}

Gate::Gate(const TimingBasic &t, const Gate &g):TimingBasic(t) {
    tag = g.tag;
    hamiltonian_tags_list = g.hamiltonian_tags_list;
    hamiltonian_obj_map_ptr = g.hamiltonian_obj_map_ptr;
}

Gate::Gate(nlohmann::json gate_config) {
    tag = gate_config["tag"];
    std::vector<std::string> h_tag_list = gate_config["hamiltonians"];
    hamiltonian_tags_list = h_tag_list;
    this->set_pulse_width(gate_config["pulse_width"]);
}

std::vector<std::string> Gate::decode_param_str(std::string params) {
    auto param_list = str_split(params,',');
    return param_list;
}

TimingDesc Gate::description() {
    TimingDesc desc = TimingBasic::description();
    std::string name_str = std::string(12,'-');
    name_str.replace(0,tag.size(),tag);
    desc.name = name_str;
    return desc;
}

/**********************************************************************************************************************/

std::vector<std::string> FID::decode_param_str(std::string params) {
    //TODO: Regex for pure number input (Now only for T/n format)
    auto param_list = Gate::decode_param_str(params);
    std::string tau_divided_str = param_list.at(0);
    auto expression = str_split(tau_divided_str,'/');
    double divider = std::stod(expression[1]);
    set_pulse_width(tau/divider);
    return param_list;
}


RTTR_REGISTRATION {
    rttr::registration::class_<Gate>("Gate").
            property("hamiltonian_tags_list",&Gate::hamiltonian_tags_list);

    rttr::registration::class_<FID>("FID").
            property("tau",&FID::tau);
};