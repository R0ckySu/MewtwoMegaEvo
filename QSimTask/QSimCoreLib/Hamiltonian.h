//
// Created by Rocky Su on 16/9/20.
//
#ifndef MEWTWOMEGAEVO_Hamiltonian_H
#define MEWTWOMEGAEVO_Hamiltonian_H

#include <armadillo>
#include <rttr/rttr_enable.h>
#include "nlohmann/json.hpp"
#include "Utils.h"
#include "ExtSigCache.h"

typedef std::string hamiltonian_tag_type;

class Hamiltonian {
public:
    hamiltonian_tag_type tag;

    double step_size;
    int num_of_steps;

    arma::vec times_vec;
    arma::vec switching_signal;

    double amplitude;
    symbolic_matrix h_mat;
    arma::cx_vec wave_form;
    std::string external_waveform_path;
    ExtSigCache* ext_sig_cache;

    Hamiltonian();
    explicit Hamiltonian(nlohmann::json h_config, std::string config_path);
    Hamiltonian(const Hamiltonian &h);
    ~Hamiltonian();

    virtual void add_signal(arma::vec _sig);
    virtual void load_waveform();   //Generating waveform
    virtual void fetch_H(arma::cx_cube* H0);
    virtual void load_ext_waveform(int param_index);
    virtual std::string description();

    virtual Hamiltonian* clone();

    virtual void clean_up_on_reload();
RTTR_ENABLE();
};

/**********************************************************************************************************************/

class Static_Hamiltonian: public Hamiltonian {
public:
    explicit Static_Hamiltonian(nlohmann::json h_config, std::string config_path);
    Static_Hamiltonian* clone();
    void load_waveform();
    void fetch_H(arma::cx_cube *H0);
};

/**********************************************************************************************************************/
class Gated_Hamiltonian: public Hamiltonian {
public:
    Gated_Hamiltonian();
    explicit Gated_Hamiltonian(nlohmann::json h_config, std::string config_path);
    Gated_Hamiltonian(const Hamiltonian &h, const Gated_Hamiltonian &g);

    double rising_time;
    double falling_time;

    void add_signal(arma::vec _sig) override;
    void load_waveform() override;
    void fetch_H(arma::cx_cube* H0) override;
    void clean_up_on_reload() override;
    std::string description() override;
    Gated_Hamiltonian* clone() override;
};


/**********************************************************************************************************************/

class MW_Hamiltonian: public Gated_Hamiltonian {
public:
    MW_Hamiltonian();
    explicit MW_Hamiltonian(nlohmann::json h_config, std::string config_path);
    MW_Hamiltonian(const Gated_Hamiltonian &g, const MW_Hamiltonian &m);
    ~MW_Hamiltonian();
    double freq;
    double phase;

    void load_waveform() override;
    void fetch_H(arma::cx_cube* H0) override;
    void clean_up_on_reload() override;
    std::string description() override;
    MW_Hamiltonian* clone() override;

RTTR_ENABLE(Gated_Hamiltonian);
};

/**********************************************************************************************************************/

class AWG_Hamiltonian: public Gated_Hamiltonian {
public:
    AWG_Hamiltonian();
    explicit AWG_Hamiltonian(nlohmann::json h_config, std::string config_path);
    AWG_Hamiltonian(const Gated_Hamiltonian &g, const AWG_Hamiltonian &a);

    void load_waveform() override;
    void fetch_H(arma::cx_cube* H0) override;
    void clean_up_on_reload() override;
    std::string description() override;
    AWG_Hamiltonian* clone() override;

RTTR_ENABLE(Gated_Hamiltonian);
};

/**********************************************************************************************************************/

class Noise_Hamiltonian: public Hamiltonian {
public:
    double shift_time;
    bool rand_shift = true;
    double randomStartPosFactor;
    Noise_Hamiltonian();
    Noise_Hamiltonian(const Hamiltonian &h, const Noise_Hamiltonian &n);
    explicit Noise_Hamiltonian(nlohmann::json noise_config, std::string config_path);
    ~Noise_Hamiltonian();

    void fetch_H(arma::cx_cube *H0) override;
    void load_ext_waveform(int param_index) override;
    std::string description() override;
    Noise_Hamiltonian* clone() override;
RTTR_ENABLE(Hamiltonian);
};

#endif




