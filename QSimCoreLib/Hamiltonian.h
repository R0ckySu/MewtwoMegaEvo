//
// Created by Rocky Su on 16/9/20.
//

#include <armadillo>

typedef std::string hamiltonian_tag_type;

class Hamiltonian {
public:

    hamiltonian_tag_type tag;

    double step_size;
    int num_of_steps;

    double amplitude;
    arma::cx_mat h_mat;
    arma::cx_vec wave_form;
    arma::vec times_vec;
    std::string external_waveform_path="";

    Hamiltonian(const Hamiltonian &h);
    ~Hamiltonian();

    virtual arma::cx_vec load_waveform();   //Generating waveform
    virtual void fetch_H(arma::cx_cube* H0);
    virtual void load_ext_waveform(std::string datapath);
private:

};

class Static_Hamiltonian: public Hamiltonian {

};

class MW_Hamiltonian: public Hamiltonian {
public:
    ~MW_Hamiltonian();
    MW_Hamiltonian(const Hamiltonian &h, const MW_Hamiltonian &m);

    struct gaussian_modulation_param {double mu; double sigma;};
    enum modulation_type {
        gaussian,
        undef
    };
    gaussian_modulation_param gaussian_mod_param;
    modulation_type modulation=undef;

    double freq;
    double phase;
    arma::vec switching_signal;

    virtual arma::cx_vec load_waveform() override;
    void fetch_H(arma::cx_cube* H0) override;

private:
    double get_amplitude(double time) const;
};

class AWG_Hamiltonian: public Hamiltonian {
    virtual arma::cx_vec load_waveform() override;
};

class Noise_Hamiltonian: public Hamiltonian {
public:
    std::string tag;
    double delta_t;
    int noise_id;
    std::vector<double> randomStartPosFactor;

public:
    virtual arma::cx_vec load_waveform() override;
    virtual void fetch_noise(arma::cx_cube *H0);

    virtual std::string get_description();
    virtual void save_noise_amp(int ensemble_idx);
    //Returns a matrix of all noise generated during simulation. Rows expands time, columns expands ensemble.
    virtual arma::vec get_averaged_noise_history();
};


