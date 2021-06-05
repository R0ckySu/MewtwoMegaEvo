//
// Created by Rocky Su on 15/12/20.
//

#ifndef MEWTWOMEGAEVO_NOISEBASIC_H
#define MEWTWOMEGAEVO_NOISEBASIC_H

#include "iostream"
#include <nlohmann/json.hpp>
#include <armadillo>

#define NOISE_MODE_ARB "arb"
#define NOISE_MODE_COLORED "colored"

class NoiseBasic {
    std::string config_file_path;
    nlohmann::json config;

    std::string export_dir;
    std::string tag;
    double time_step;
    int noise_length;
    double alpha;
    double amp;
    int channels;
    int start_idx=0;

    std::string noise_gen_mode  = NOISE_MODE_COLORED;
    std::string arb_noise_expr = "";

    void generate_colored_noise_amp_func();
    void generate_arb_noise_amp_func();
    void gen_reamped_white_noise_with_spec_func(arma::cx_vec amp_func);

public:
    void load_config_from_path(std::string config_path);
    void generate_noise();

};


#endif //MEWTWOMEGAEVO_NOISEBASIC_H
