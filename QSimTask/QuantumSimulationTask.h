//
// Created by Rocky Su on 18/9/20.
//

#include <vector>
#include <stack>
#include <nlohmann/json.hpp>
#include <rttr/rttr_enable.h>

#include "QSimCoreLib/QSimCore.h"
#include "ParamScheduler.h"
#include "SimPrototypes.h"
#include "QSimCoreLib/ExtSigCache.h"

#define CONFIG_FOLDER_NAME "config_files"
#define OUTPUT_FOLDER_NAME "sim_results"

class QSimTask {
    RTTR_ENABLE();
public:
    int num_job_group;
    int job_id;
    std::string job_slicing_strategy;

    int system_dimension=2;
    std::string task_name;
    int log_level_threshold=1;
    std::string task_time_stamp;
    std::string config_file_folder;
    std::string result_output_folder;
    ExtSigCache noise_sig_cache;

    nlohmann::json sim_configs;
    nlohmann::json gate_configs;
    nlohmann::json hamiltonian_configs;

    bool enable_param_parallel_mode = false;
    bool will_record_unitary;
    bool will_record_all_measurement;
    bool will_record_density_mat;
    double step_size;
    int iterations;
    std::vector<symbolic_matrix> rho_inits;
    std::vector<symbolic_matrix> observables;
    ParamScheduler param_schedule;
    SimPrototypes simulation_prototypes;

    std::string result_exact_path;

public:
    QSimTask();
    ~QSimTask();
    virtual void load_sim_configs();
    virtual void load_gate_configs();
    virtual void load_hamiltonian_configs();
    void preload() {
        load_sim_configs();
        load_hamiltonian_configs();
        load_gate_configs();
    }

    virtual SimPrototypes* reload_prototypes_with_sweeping_parameter(int index);
    virtual void sweeping_repeat_parallel();
    virtual void sweeping_param_parallel();
    virtual void launch_task();

    arma::cx_cube* compile_time_dep_ctrl_hamiltonian(std::map<hamiltonian_tag_type, Hamiltonian *> hamiltonian_prototype_map, std::map<gate_tag_type, Gate *> gate_map ,Sequence seq);
    arma::cx_cube* compile_time_dep_noise_hamiltonian(std::map<hamiltonian_tag_type, Noise_Hamiltonian *> hamiltonian_prototype_map,int noise_idx, std::vector<double> random_start_pos_factor, int total_num_steps);
private:
    static nlohmann::json load_config_from_path(const std::string& path);
    void task_log(std::string message, int log_level);

    std::string get_result_file_name();
};
