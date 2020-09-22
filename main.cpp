#include <iostream>
#include "QSimTask/QuantumSimulationTask.h"

int main() {
    QSimTask task = QSimTask();
    task.config_file_folder = "/Users/rockysu/MewtwoMegaEvo/configfiles";
    task.preload();
    std::cout << "Hello, World!" << std::endl;
    return 0;
}
