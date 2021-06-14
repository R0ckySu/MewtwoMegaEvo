//
// Created by Rocky Su on 17/9/20.
//

#include "Gate.h"
#include "Utils.h"
#include <rttr/registration.h>
#include <exprtk/exprtk.hpp>

Gate::Gate() {
    tag = "";
    hamiltonian_tags_list = std::vector<std::string>();
}

Gate::Gate(const TimingBasic &t, const Gate &g):TimingBasic(t) {
    tag = g.tag;
    gate_amp = g.gate_amp;
    hold_on = g.hold_on;
    hamiltonian_tags_list = g.hamiltonian_tags_list;
    ext_shaped_sig_path = g.ext_shaped_sig_path;
    ext_shaped_sig = g.ext_shaped_sig;
}

Gate *Gate::clone() {
    return new Gate(*this);
}

Gate::Gate(nlohmann::json gate_config, double _step_size) {
    step_size = _step_size;
    tag = gate_config["tag"];
    hold_on = gate_config["hold_on"];
    std::vector<std::string> h_tag_list = gate_config["hamiltonians"];
    hamiltonian_tags_list = h_tag_list;
    ext_shaped_sig_path = gate_config["ext_shaped_sig_path"];
    if (!ext_shaped_sig_path.empty()) {
        ext_shaped_sig = arma::vec().load(ext_shaped_sig_path);
        double ext_sig_pulse_width = step_size * ext_shaped_sig.size();
        this->set_pulse_width(ext_sig_pulse_width);
    } else {
        this->set_pulse_width(gate_config["pulse_width"]);
    }
}

std::vector<std::string> Gate::decode_param_str(std::string params) {
    auto param_list = str_split(params,',');

    exprtk::expression<double> expression_T;
    std::string T_expression_str = param_list.at(T_pos);

    exprtk::symbol_table<double> symbol_table_T;
    double T = get_pulse_width();
    symbol_table_T.add_variable("T", T);
    expression_T.register_symbol_table(symbol_table_T);

    exprtk::parser<double> parser_T;
    parser_T.compile(T_expression_str,expression_T);
    set_pulse_width(expression_T.value());

    if (param_list.size() >= Amp_pos+1) {
        exprtk::expression<double> expression_A;
        std::string A_expression_str = param_list.at(Amp_pos);

        exprtk::symbol_table<double> symbol_table_A;
        double Amp_temp = 1;
        symbol_table_A.add_variable("A", Amp_temp);
        expression_A.register_symbol_table(symbol_table_A);

        exprtk::parser<double> parser_A;
        parser_A.compile(A_expression_str,expression_A);
        gate_amp = expression_A.value();
    }

    return param_list;
}

TimingDesc Gate::description() {
    TimingDesc desc = TimingBasic::description();
    std::string name_str = std::string(12,'-');
    name_str.replace(0,tag.size(),tag);
    desc.name = name_str;

    desc.table_head.append("-----Amp---|");
    std::stringstream descStream;
    descStream << std::setw(12) << std::scientific << std::setprecision(4) << gate_amp << "|";
    desc.data_row.append(descStream.str());
    return desc;
}

/**********************************************************************************************************************/

//std::vector<std::string> FID::decode_param_str(std::string params) {
//    //TODO: Regex for pure number input (Now only for T/n format)
//    auto param_list = Gate::decode_param_str(params);
//
//    std::string tau_divided_str = param_list.at(0);
//    auto expression = str_split(tau_divided_str,'/');
//    double divider = std::stod(expression[1]);
//    set_pulse_width(tau/divider);
//    return param_list;
//}

/**********************************************************************************************************************/

MeasurementMarker::MeasurementMarker() {
    hamiltonian_tags_list = std::vector<std::string>();
    tag = "M";
    start_time = 0;
    end_time = 0;
    pulse_width = 0;
}

RTTR_REGISTRATION {
    rttr::registration::class_<Gate>("Gate").
            property("hamiltonian_tags_list",&Gate::hamiltonian_tags_list);
};