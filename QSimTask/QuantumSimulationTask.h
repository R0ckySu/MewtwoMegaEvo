//
// Created by Rocky Su on 18/9/20.
//

#include <vector>
#include <stack>
#include <nlohmann/json.hpp>

#include "QSimCoreLib/Sequence.h"
#include "QSimCoreLib/VonNeumannSolver.h"

#include <rttr/rttr_enable.h>

enum TaskStatus{
    Init,
    Preloaded,
    Finished,
    Recorded,
};

enum MeasurementRecordMode {
    End_of_Each_Pulse,
    End_of_Each_Repeat_Cycle,
    Complete,
};

class QSimTask {
    RTTR_ENABLE();
public:
    double step_size;
    int iterations;
    TaskStatus status;
    std::string config_file_folder;

    nlohmann::json sim_configs;
    nlohmann::json gate_configs;
    nlohmann::json hamiltonian_configs;

    std::map<gate_tag_type, Gate *> gate_prototype_map;
    std::map<hamiltonian_tag_type, Hamiltonian *> ctrl_hamiltonian_prototype_map;
    std::map<hamiltonian_tag_type, Noise_Hamiltonian *> noise_hamiltonian_prototype_map;
    std::vector<arma::cx_mat> rho_inits;
    arma::vec  param_vec;
private:
//    void load_sequence_to_solver();
//    void load_noise_to_solver();
//    void load_measurements_to_solver();
//    void launch_solver();
//    void measument_solver();

public:
    QSimTask();
    ~QSimTask();
    virtual void load_sim_configs();
    virtual void load_gate_configs();
    virtual void load_hamiltonian_configs();

    virtual void reload_with_sweeping_parameter(int index);
    virtual Sequence* load_sequence();
    virtual VonNeumannSolver* launch_solver(int total_num_steps,int matrix_dim);
    void measument_solver();

    void preload() {
        std::cout << "QSimTask: Loading simulation configs\n" << std::endl;
        load_sim_configs();
        std::cout << "QSimTask: Loading Hamiltonian prototypes\n" << std::endl;
        load_hamiltonian_configs();
        std::cout << "QSimTask: Loading Gate prototypes\n" << std::endl;
        load_gate_configs();
    }

    void sweeping_task() {
        std::cout << "QSimTask: Parametric Sweeping started\n" << std::endl;
        //TODO: Grouped parameters for job slicing.
        for (int i = 0; i < param_vec.size(); ++i) {
            reload_with_sweeping_parameter(i);
            Sequence* seq = load_sequence();
            launch_solver(seq->get_total_num_steps(),sim_configs["system_dim"]);
            std::cout << "QSimTask: Solver job done!\n" << std::endl;
            measument_solver();
            std::cout << "QSimTask: Finished measurements!\n" << std::endl;
        }
    }

private:
    static nlohmann::json load_config_from_path(const std::string& path);
};
