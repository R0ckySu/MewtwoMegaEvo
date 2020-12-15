//
// Created by Rocky Su on 15/12/20.
//

#ifndef MEWTWOMEGAEVO_NOISEBASIC_H
#define MEWTWOMEGAEVO_NOISEBASIC_H

#include "iostream"
#include <nlohmann/json.hpp>

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
public:
    void load_config_from_path(std::string config_path);
    void generate_colored_noise();
};


#endif //MEWTWOMEGAEVO_NOISEBASIC_H
