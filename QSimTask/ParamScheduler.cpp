//
// Created by Rocky Su on 6/11/20.
//

#include "ParamScheduler.h"
#include "QSimCoreLib/Utils.h"
#include <fstream>

ParamScheduler::ParamScheduler() {
    num_of_params = 0;
    param_info_table = std::vector<std::map<std::string, std::string>>();
    param_val_vec_map = std::map<std::string, arma::vec>();
    param_string_vec_map = std::map<std::string, std::vector<std::string>>();
}

void ParamScheduler::load_param_info_table_from_json(std::vector<nlohmann::json> param_info_json_list) {
    for (int i = 0; i < param_info_json_list.size(); ++i) {
        nlohmann::json prama_info_item = param_info_json_list.at(i);
        auto param_item_map = std::map<std::string, std::string>();
        param_item_map.insert(std::make_pair("class",std::string(prama_info_item["class"])));
        param_item_map.insert(std::make_pair("tag",std::string(prama_info_item["tag"])));
        param_item_map.insert(std::make_pair("prop",std::string(prama_info_item["property"])));

        if (prama_info_item.find("val_file") != prama_info_item.end()) {
            param_item_map.insert(std::make_pair("val_file",std::string(prama_info_item["val_file"])));
        } else if (prama_info_item.find("string_file") != prama_info_item.end()) {
            param_item_map.insert(std::make_pair("string_file",std::string(prama_info_item["string_file"])));
        } else {
            std::cout<< "ParamScheduler: Error! Can not find parameter file name for tag: " << prama_info_item["tag"] << ", neither with key val_file nor string_file!" << std::endl;
        }
        param_info_table.push_back(param_item_map);
    }
}

std::string ParamScheduler::get_param_string_for_ith_param(int param_idx) {
    std::string param_str;
    for (int i = 0; i < param_info_table.size(); ++i) {
        if (param_info_table.at(i).find("val_file") != param_info_table.at(i).end()) {
            std::string val_name = param_info_table.at(i)["val_file"];
            std::string val_string = double_to_fixprecision_str(param_val_vec_map.at(val_name).at(param_idx), 4);
            param_str.append("#").append(val_name).append("=").append(val_string);
        } else if(param_info_table.at(i).find("string_file") != param_info_table.at(i).end()) {
            std::string string_file_name = param_info_table.at(i)["string_file"];
            std::string str_sym = param_string_vec_map.at(string_file_name).at(param_idx);
            param_str.append("#").append(string_file_name).append("=").append(str_sym);
        }
    }
    return param_str;
}

void ParamScheduler::load_param_from_file(std::string folder) {
    for (auto param_info_item : param_info_table) {
        if (param_info_item.find("val_file") != param_info_item.end()) {
            std::string file_path = std::string(folder).append("/").append(param_info_item.at("val_file"));
            arma::vec val = arma::vec();
            val.load(file_path,arma::csv_ascii);
            param_val_vec_map.insert(std::make_pair(param_info_item.at("val_file"), val));
            num_of_params = val.n_elem;
        } else if (param_info_item.find("string_file") != param_info_item.end()) {
            std::string file_path = std::string(folder).append("/").append(param_info_item.at("string_file"));
            std::fstream sym_str_file_stream = std::fstream(file_path);
            std::string str_temp;
            std::vector<std::string> sym_string_list = std::vector<std::string>();
            while (getline(sym_str_file_stream,str_temp)){
                sym_string_list.push_back(str_temp);
            }
            param_string_vec_map.insert(std::make_pair(param_info_item.at("string_file"), sym_string_list));
        }
    }
}

void ParamScheduler::process_prameter_vec_with_job_slicing_strategy(int job_id, int num_job_group, std::string job_slicing_strategy) {
    if ((num_job_group > num_of_params) || (job_id>num_job_group)) {
        num_job_group = 1;
        job_id = 0;
        std::cout << std::string("Wrong slicing parameters!") << std::endl;
        return;
    }

    int start_pos_for_this_job = 0;
    int end_pos_for_this_job = 0;

    if (job_slicing_strategy == JOB_SLICING_LINSPACE) {
        arma::vec index_ends_list = arma::linspace(0,num_of_params,num_job_group+1);
        start_pos_for_this_job = index_ends_list.at(job_id);
        end_pos_for_this_job = index_ends_list.at(job_id+1) -1;
    } else if (job_slicing_strategy == JOB_SLICING_LOGSPACE) {
        arma::vec index_ends_list = arma::round(arma::logspace(0,log10(num_of_params),num_job_group));
        index_ends_list.insert_rows(0,1);
        for (int i = 1; i < index_ends_list.size()-1; ++i) {
            if (index_ends_list.at(i) == index_ends_list.at(i-1)) {
                index_ends_list.at(i) = index_ends_list.at(i)+1;
            }
        }
        start_pos_for_this_job = index_ends_list.at(job_id);
        end_pos_for_this_job = index_ends_list.at(job_id+1)-1;
    } else if (job_slicing_strategy == JOB_SLICING_INVLOGSPACE) {
        arma::vec index_ends_list = arma::round(arma::logspace(0,log10(num_of_params),num_job_group));
        index_ends_list.insert_rows(0,1);
        for (int i = 1; i < index_ends_list.size()-1; ++i) {
            if (index_ends_list.at(i) == index_ends_list.at(i-1)) {
                index_ends_list.at(i) = index_ends_list.at(i)+1;
            }
        }
        index_ends_list = num_of_params - index_ends_list;
        start_pos_for_this_job = index_ends_list.at(job_id);
        end_pos_for_this_job = index_ends_list.at(job_id+1)-1;
    }

    for (auto param_val_item : param_val_vec_map) {
        arma::vec sliced_vec = param_val_item.second.subvec(start_pos_for_this_job,end_pos_for_this_job);
        param_val_item.second = sliced_vec;
        std::stringstream param_str;
        param_str << sliced_vec;
        std::cout<< std::string("Job sliced, ").append(param_val_item.first).append(": param vec:\n").append(param_str.str()) << std::endl;
    }
}
