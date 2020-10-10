//
// Created by Rocky Su on 17/9/20.
//

#ifndef MEWTWOMEGAEVO_Gate_H
#define MEWTWOMEGAEVO_Gate_H

#include <armadillo>
#include "TimingBasic.h"
#include "nlohmann/json.hpp"
#include "Hamiltonian.h"
#include <rttr/rttr_enable.h>

enum GateParamPosMap{
    T_pos = 0,
};

typedef std::string gate_tag_type;

class Gate: public TimingBasic {
public:
    //Initialiser
    Gate();
    explicit Gate(nlohmann::json gate_config);
    //Copy constructor
    Gate(const TimingBasic &t,const Gate &g);

    gate_tag_type tag;
    std::vector<hamiltonian_tag_type> hamiltonian_tags_list;
    virtual std::vector<std::string> decode_param_str(std::string params);
    virtual TimingDesc description() override;
    RTTR_ENABLE(TimingBasic);
};

class MeasurementMarker: public Gate {
public:
    MeasurementMarker();
};

#endif