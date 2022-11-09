//
// Created by Rocky Su on 8/10/20.
//

#ifndef MEWTWOMEGAEVO_COMMAND_ARG_PARSER_H
#define MEWTWOMEGAEVO_COMMAND_ARG_PARSER_H

#include <iostream>
#include <getopt.h>
#include "nlohmann/json.hpp"
#include "QSimTask/QuantumSimulationTask.h"

const char* short_options = "g:i:o:c:t:r";
const struct option long_options[] = {
        {"num_job_group", 1, NULL,'g'},
        {"job_id",        1, NULL,'i'},
        {"output_folder", 1, NULL,'o'},
        {"config_folder", 1, NULL,'c'},
        {"timestamp",     1, NULL, 't'},
        {"resume_from_idx",1, NULL, 'r'},
        {nullptr,         0, nullptr, 0}
};

QSimTask* create_task_from_command_arg_parser(int argc, char *argv[]){
    QSimTask *sim_task_new = new QSimTask();
    int c;
    while((c = getopt_long(argc, argv, short_options, long_options, nullptr)) != -1) {
        printf("[+]Get sim task startup options : %d \n", c);
        switch (c) {
            case 'g':
                std::cout << "Num of jobs:" << optarg << std::endl;
                sim_task_new->num_job_group = std::stoi(optarg);
                break;
            case 'i':
                std::cout << "Index of job:" << optarg << std::endl;
                sim_task_new->job_id = std::stoi(optarg);
                break;
            case 'o':
                std::cout << "Export Folder:" << optarg << std::endl;
                sim_task_new->result_output_folder = optarg;
                break;
            case 'c':
                std::cout << "Config File Path:" << optarg << std::endl;
                sim_task_new->config_file_folder = optarg;
                break;
            case 't':
                std::cout << "Time stamp:" << optarg << std::endl;
                sim_task_new->task_time_stamp = optarg;
                break;
            case 'r':
                std::cout << "Resumed from index:" << optarg << std::endl;
                sim_task_new->resume_from_idx = std::stoi(optarg);
                break;
            default:
                break;
        }
    }
    return sim_task_new;
};

#endif //MEWTWOMEGAEVO_COMMAND_ARG_PARSER_H
