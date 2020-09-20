//
// Created by Rocky Su on 16/9/20.
//

#include <armadillo>

typedef std::string hamiltonian_tag_type;

class Hamiltonian {
public:

    hamiltonian_tag_type tag;
    int parametric_index;

    double step_size;
    int num_of_steps;
    arma::vec switching_signal;

    double amplitude;
    arma::cx_mat h_mat;
    arma::cx_vec wave_form;
    arma::vec times_vec;
    std::string external_waveform_path;

    Hamiltonian();
    Hamiltonian(const Hamiltonian &h);
    ~Hamiltonian();

    virtual void load_waveform();   //Generating waveform
    virtual void fetch_H(arma::cx_cube* H0);
    virtual void load_ext_waveform(std::string datapath);
private:

};

/**********************************************************************************************************************/

class Static_Hamiltonian: public Hamiltonian {

};

/**********************************************************************************************************************/

class MW_Hamiltonian: public Hamiltonian {
public:
    MW_Hamiltonian();
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

    virtual void load_waveform() override;
    void fetch_H(arma::cx_cube* H0) override;

private:
    double get_amplitude(double time) const;
};

/**********************************************************************************************************************/

class AWG_Hamiltonian: public Hamiltonian {
//    virtual arma::cx_vec load_waveform() override;
};

/**********************************************************************************************************************/

class Noise_Hamiltonian: public Hamiltonian {
//public:
//    std::vector<double> randomStartPosFactor;
//
//public:
//    virtual arma::cx_vec load_waveform() override;
};


