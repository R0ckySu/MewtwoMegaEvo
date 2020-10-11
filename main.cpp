#include <iostream>
#include <armadillo>
#include "command_arg_parser.h"

int main(int argc, char *argv[]) {
    QSimTask *sim_task = create_task_from_command_arg_parser(argc,argv);
    sim_task->preload();
    sim_task->sweeping_task();
    return 0;
}