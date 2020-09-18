//
// Created by Rocky Su on 17/9/20.
//

#include <armadillo>
#include "TimingBasic.h"

typedef std::string gate_tag_type;

class Gate: public TimingBasic {
public:
    //Initialiser
    Gate();
    //Copy constructor
    Gate(const TimingBasic &t,const Gate &g);

    gate_tag_type tag;
    std::vector<std::string> hamiltonian_tags_list;
};
