//
// Created by Rocky Su on 6/11/20.
//

#ifndef MEWTWOMEGAEVO_PARAMSCHEDULER_H
#define MEWTWOMEGAEVO_PARAMSCHEDULER_H

#include <iostream>
#include <vector>
#include <armadillo>
#include <nlohmann/json.hpp>

#define JOB_SLICING_LOGSPACE "logspace"
#define JOB_SLICING_INVLOGSPACE "inv_logspace"
#define JOB_SLICING_LINSPACE "linspace"

/*
 * ParamScheduler
 *  - Providing logic for decoding the parametric sweeping information
 *  - Providing logic for slicing the parameter vector with job slicing strategy.
 * */
struct ParamScheduler {
    ParamScheduler();
    int num_of_params;
    std::vector<std::map<std::string, std::string>> param_info_table;
    std::map<std::string, arma::vec> param_val_map;

    void load_param_info_table_from_json(std::vector<nlohmann::json> param_info_json_list);
    void load_param_val_from_folder(std::string folder);
    void process_prameter_vec_with_job_slicing_strategy(int job_id, int num_job_group, std::string job_slicing_strategy);
    std::string get_param_val_string_for_ith_param(int param_idx);
};

#endif //MEWTWOMEGAEVO_PARAMSCHEDULER_H
