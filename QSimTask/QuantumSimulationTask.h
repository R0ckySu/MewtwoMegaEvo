//
// Created by Rocky Su on 18/9/20.
//

#include <vector>
#include <stack>
#include <nlohmann/json.hpp>

#include "QSimCoreLib/Sequence.h"
#include "QSimCoreLib/VonNeumannSolver.h"

#include <rttr/rttr_enable.h>

class QSimTask {
    RTTR_ENABLE();
public:
    int log_level_threshold=1;
    std::string task_name;
    std::string task_time_stamp;
    std::string config_file_folder;
    nlohmann::json sim_configs;
    nlohmann::json gate_configs;
    nlohmann::json hamiltonian_configs;

    double step_size;
    int iterations;
    std::vector<arma::cx_mat> rho_inits;
    arma::vec  param_vec;
    std::map<gate_tag_type, Gate *> gate_prototype_map;
    std::map<hamiltonian_tag_type, Hamiltonian *> ctrl_hamiltonian_prototype_map;
    std::map<hamiltonian_tag_type, Noise_Hamiltonian *> noise_hamiltonian_prototype_map;

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
        load_sim_configs();
        load_hamiltonian_configs();
        load_gate_configs();
    }

    void sweeping_task() {
        std::cout << "QSimTask: Parametric Sweeping started\n" << std::endl;
        //TODO: Grouped parameters for job slicing.
        for (int i = 0; i < param_vec.size(); ++i) {
            reload_with_sweeping_parameter(i);
            Sequence* seq = load_sequence();
            launch_solver(seq->get_total_num_steps(),sim_configs["system_dim"]);
            measument_solver();
        }
    }

private:
    static nlohmann::json load_config_from_path(const std::string& path);
    void task_log(std::string message, int log_level);
};
