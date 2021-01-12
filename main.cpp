#include <iostream>
#include <armadillo>
#include "command_arg_parser.h"

int main(int argc, char *argv[]) {
    QSimTask *sim_task = create_task_from_command_arg_parser(argc,argv);
    sim_task->preload();
    sim_task->launch_task();

//    std::string matlab_cmd = std::string("matlab -nodisplay -r \"LoadMeasMarkerDataSet('").append(sim_task->result_exact_path).append("');exit\"");
//    std::cout << matlab_cmd << std::endl;
//    system(matlab_cmd.c_str());

    std::string matlab_cmd = std::string("Please run in matlab:\n \"LoadMeasMarkerData('").append(sim_task->result_exact_path).append("')\"");
    std::cout << matlab_cmd << std::endl;

    return 0;
}