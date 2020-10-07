//
// Created by Rocky Su on 16/9/20.
//

#include <armadillo>
#include "nlohmann/json.hpp"

#include <rttr/rttr_enable.h>

typedef std::string hamiltonian_tag_type;

class Hamiltonian {
public:
    hamiltonian_tag_type tag;

    double step_size;
    int num_of_steps;

    arma::vec times_vec;
    arma::vec switching_signal;

    double amplitude;
    arma::cx_mat h_mat;
    arma::cx_vec wave_form;
    std::string external_waveform_path;

    Hamiltonian();
    explicit Hamiltonian(nlohmann::json h_config);
    Hamiltonian(const Hamiltonian &h);
    ~Hamiltonian();

    virtual void load_waveform();   //Generating waveform
    virtual void fetch_H(arma::cx_cube* H0);
    virtual void load_ext_waveform(int param_index);
    virtual std::string description();
RTTR_ENABLE();
};

/**********************************************************************************************************************/

class Static_Hamiltonian: public Hamiltonian {
public:
    explicit Static_Hamiltonian(nlohmann::json h_config);
};

/**********************************************************************************************************************/

class MW_Hamiltonian: public Hamiltonian {
public:
    MW_Hamiltonian();
    explicit MW_Hamiltonian(nlohmann::json h_config);
//    ~MW_Hamiltonian();
    MW_Hamiltonian(const Hamiltonian &h, const MW_Hamiltonian &m);

    struct gaussian_modulation_param {double mu; double sigma;};
    enum modulation_type {
        gaussian,
        undef
    };
    gaussian_modulation_param gaussian_mod_param{};
    modulation_type modulation=undef;

    double freq;
    double phase;

    void load_waveform() override;
    void fetch_H(arma::cx_cube* H0) override;
    std::string description() override;

private:
    double get_amplitude(double time) const;

RTTR_ENABLE(Hamiltonian);
};

/**********************************************************************************************************************/

class AWG_Hamiltonian: public Hamiltonian {
//    virtual arma::cx_vec load_waveform() override;
RTTR_ENABLE(Hamiltonian);
};

/**********************************************************************************************************************/

class Noise_Hamiltonian: public Hamiltonian {
public:
    double shift_time;
    std::vector<double> randomStartPosFactor;
    Noise_Hamiltonian();
    Noise_Hamiltonian(const Hamiltonian &h, const Noise_Hamiltonian &n);
    explicit Noise_Hamiltonian(nlohmann::json noise_config);
    ~Noise_Hamiltonian();

    void fetch_H(arma::cx_cube *H0) override;

RTTR_ENABLE(Hamiltonian);
//public:
//    virtual arma::cx_vec load_waveform() override;
};




