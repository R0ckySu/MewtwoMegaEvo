//
// Created by Rocky Su on 17/9/20.
//

#include "Gate.h"
#include "Utils.h"
#include <map>
#include <rttr/registration.h>
#include <exprtk/exprtk.hpp>

Gate::Gate() {
    tag = "";
    hamiltonian_tags_list = std::vector<std::string>();
}

Gate::Gate(const TimingBasic &t, const Gate &g):TimingBasic(t) {
    tag = g.tag;
    amp = g.amp;
    hamiltonian_tags_list = g.hamiltonian_tags_list;
    seq_shift_time = g.seq_shift_time;
    ext_shaped_sig_path = g.ext_shaped_sig_path;
    ext_shaped_sig = g.ext_shaped_sig;
}

Gate *Gate::clone() {
    return new Gate(*this);
}

Gate::Gate(nlohmann::json gate_config, double _step_size) {
    step_size = _step_size;
    tag = gate_config["tag"];
    seq_shift_time = gate_config["shift_time"];
    std::vector<std::string> h_tag_list = gate_config["hamiltonians"];
    hamiltonian_tags_list = h_tag_list;
    ext_shaped_sig_path = gate_config["ext_shaped_sig_path"];
    if (!ext_shaped_sig_path.empty()) {
        std::cout << "Gate: " << tag << " load shaped sig from: " << ext_shaped_sig_path << std::endl;
        ext_shaped_sig = arma::vec();
        ext_shaped_sig.load(ext_shaped_sig_path, arma::file_type::csv_ascii);
        double ext_sig_pulse_width = step_size * (ext_shaped_sig.n_elem);
        this->set_pulse_width(ext_sig_pulse_width);
//        std::cout << "Gate: " << tag << " shaped signal:\n " << ext_shaped_sig << std::endl;
    } else {
        this->set_pulse_width(gate_config["pulse_width"]);
    }
}

std::vector<std::string> Gate::decode_param_str(std::string params) {
    std::vector<std::string> param_list = str_split(params,',');

    std::map<std::string, std::string> express_str_map;
    for (int i = 0; i < param_list.size(); i++) {
        if (param_list.at(i).find('A') != std::string::npos) {
            express_str_map.insert(std::make_pair(std::string("A"), param_list.at(i)));
        }
        if (param_list.at(i).find('T') != std::string::npos) {
            express_str_map.insert(std::make_pair(std::string("T"), param_list.at(i)));
        }
    }

    if (express_str_map.count("T") > 0) {
        exprtk::expression<double> expression_T;
        std::string T_str_exp = express_str_map["T"];
//        std::cout << "Gate: " << tag << " expression:" << T_str_exp << std::endl;
        exprtk::symbol_table<double> symbol_table;
        double T = get_pulse_width();
        symbol_table.add_variable("T", T);
        expression_T.register_symbol_table(symbol_table);
        exprtk::parser<double> parser;
        parser.compile(T_str_exp, expression_T);
        set_pulse_width(expression_T.value());
        std::cout << "Gate: " << tag << " pulse_width:" << get_pulse_width() << std::endl;
    }

    if (express_str_map.count("A") > 0) {
        exprtk::expression<double> expression_A;
        std::string A_str_exp = express_str_map["A"];
        exprtk::symbol_table<double> symbol_table_A;
        symbol_table_A.add_variable("A", amp);
        expression_A.register_symbol_table(symbol_table_A);
        exprtk::parser<double> parser;
        parser.compile(A_str_exp, expression_A);
        amp = expression_A.value();
        std::cout << "Gate: " << tag << " amp:" << amp << std::endl;
    }

    return param_list;
}

TimingDesc Gate::description() {
    TimingDesc desc = TimingBasic::description();
    std::string name_str = std::string(12,'-');
    name_str.replace(0,tag.size(),tag);
    desc.name = name_str;
    return desc;
}

Gate::~Gate() {
    arma::vec().swap(ext_shaped_sig);
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