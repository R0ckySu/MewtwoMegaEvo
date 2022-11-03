//
// Created by Rocky Su on 18/9/20.
//
#ifndef MEWTWOMEGAEVO_SEQUENCE_H
#define MEWTWOMEGAEVO_SEQUENCE_H

#include <armadillo>
#include "Gate.h"

class Sequence: public TimingBasic {
public:
    Sequence();
    ~Sequence();

    std::map<gate_tag_type, arma::vec> gate_switching_map;
    arma::vec measurement_time_point_vec;
    std::vector<std::string> active_gate_tag_list;
    void append_gate(const Gate& g);
    void append_sequence(const Sequence & seq);
    void generate_switching_sig();
    void load_sequence(double _step_size, std::string sequence_string,std::map<std::string,std::string> symbol_alias, std::map<std::string, Gate *> gate_prototype_map);
    int get_total_num_steps();
private:
    arma::vec time_vec;
    std::vector<Gate *> sequential_gate_list;
};

#endif