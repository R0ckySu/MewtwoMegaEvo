#include <iostream>
#include "QSimTask/QuantumSimulationTask.h"
#include <armadillo>

int main() {
//    std::cout << "Hello, World!\n" << qmt::spinorDecoder("XY") << std::endl;
    QSimTask task = QSimTask();
    rttr::type class_type = rttr::type::get(task);
    rttr::property config_prp = class_type.get_property("config_file_folder");
    config_prp.set_value(task,std::string("/Users/rockysu/MewtwoMegaEvo/configfiles"));
    task.result_output_folder = std::string("/Users/rockysu/MewtwoMegaEvo/configfiles");
    task.preload();
    task.sweeping_task();

    return 0;
}