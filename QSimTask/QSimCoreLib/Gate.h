//
// Created by Rocky Su on 17/9/20.
//

#ifndef MEWTWOMEGAEVO_Gate_H
#define MEWTWOMEGAEVO_Gate_H

#include <armadillo>
#include "TimingBasic.h"
#include <nlohmann/json.hpp>
#include "Hamiltonian.h"
#include <rttr/rttr_enable.h>

enum GateParamPosMap{
    T_pos = 0,
    Amp_pos = 1,
};

typedef std::string gate_tag_type;

class Gate: public TimingBasic {
public:
    //Initialiser
    Gate();
    explicit Gate(nlohmann::json gate_config, double _step_size);
    //Copy constructor
    Gate(const TimingBasic &t,const Gate &g);

    double gate_amp = 1;
    bool hold_on = false;
    gate_tag_type tag;
    std::vector<hamiltonian_tag_type> hamiltonian_tags_list;
    std::string ext_shaped_sig_path;
    arma::vec ext_shaped_sig;
    virtual std::vector<std::string> decode_param_str(std::string params);
    virtual TimingDesc description() override;
    Gate* clone() override;
    RTTR_ENABLE(TimingBasic);
};

class MeasurementMarker: public Gate {
public:
    MeasurementMarker();
};

#endif