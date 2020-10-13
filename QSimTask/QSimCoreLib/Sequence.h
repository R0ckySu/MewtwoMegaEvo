//
// Created by Rocky Su on 18/9/20.
//

#include <armadillo>
#include "Gate.h"


class Sequence: public TimingBasic {
public:
    Sequence();
    ~Sequence();

    std::map<gate_tag_type, arma::vec> gate_switching_map;
    arma::vec measurement_time_point_vec;

    void append_gate(const Gate& g);
    void append_sequence(const Sequence & seq);
    void generate_switching_sig();
    void test();
private:
    arma::vec time_vec;
    std::vector<Gate *> sequential_gate_list;
};
