//
// Created by Rocky Su on 16/9/20.
//

#include <armadillo>
#include "nlohmann/json.hpp"

typedef std::string hamiltonian_tag_type;

class Hamiltonian {
public:
    hamiltonian_tag_type tag;
    int parametric_index=0;

    double step_size;
    int num_of_steps;
    arma::vec switching_signal;

    double amplitude;
    arma::cx_mat h_mat;
    arma::cx_vec wave_form;
    arma::vec times_vec;
    std::string external_waveform_path;

    Hamiltonian();
    explicit Hamiltonian(nlohmann::json h_config);
    Hamiltonian(const Hamiltonian &h);
    ~Hamiltonian();

    virtual void load_waveform();   //Generating waveform
    virtual void fetch_H(arma::cx_cube* H0);
    virtual void load_ext_waveform(std::string datapath);
    virtual std::string description();
private:

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
};

/**********************************************************************************************************************/

class AWG_Hamiltonian: public Hamiltonian {
//    virtual arma::cx_vec load_waveform() override;
};

/**********************************************************************************************************************/

class Noise_Hamiltonian: public Hamiltonian {
public:
    Noise_Hamiltonian();
    explicit Noise_Hamiltonian(nlohmann::json noise_config);
    ~Noise_Hamiltonian();
//public:
//    std::vector<double> randomStartPosFactor;
//
//public:
//    virtual arma::cx_vec load_waveform() override;
};


