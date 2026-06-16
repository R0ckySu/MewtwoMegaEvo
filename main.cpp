#include <iostream>
#include <armadillo>
#include "command_arg_parser.h"

int main(int argc, char *argv[]) {
    try {
        QSimTask *sim_task = create_task_from_command_arg_parser(argc,argv);
        sim_task->preload();
        sim_task->launch_task();

        std::string matlab_cmd = std::string("./matlab_process.sh ").append(sim_task->result_exact_path);
        std::cout << matlab_cmd << std::endl;
        system(matlab_cmd.c_str());
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}