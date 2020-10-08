//
// Created by Rocky Su on 18/9/20.
//

#include <vector>
#include <stack>
#include <nlohmann/json.hpp>

#include "QSimCoreLib/Measurement.h"
#include "QSimCoreLib/Sequence.h"
#include "QSimCoreLib/VonNeumannSolver.h"

#include <rttr/rttr_enable.h>

class QSimTask {
    RTTR_ENABLE();
public:
    int job_group_size;
    int job_id;

    std::string task_name;
    int log_level_threshold=1;
    std::string task_time_stamp;
    std::string config_file_folder;
    std::string result_output_folder;

    nlohmann::json sim_configs;
    nlohmann::json gate_configs;
    nlohmann::json hamiltonian_configs;

    bool will_record_unitary;
    bool will_record_all_measurement;
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
    virtual std::vector<arma::cx_cube> launch_solver(int total_num_steps,int matrix_dim);
    void measument_solver();


    void preload() {
        load_sim_configs();
        load_hamiltonian_configs();
        load_gate_configs();
    }

    void sweeping_task();

private:
    static nlohmann::json load_config_from_path(const std::string& path);
    void task_log(std::string message, int log_level);
};
