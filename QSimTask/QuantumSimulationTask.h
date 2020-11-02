//
// Created by Rocky Su on 18/9/20.
//

#include <vector>
#include <stack>
#include <nlohmann/json.hpp>

#include "QSimCoreLib/QSimCore.h"

#include <rttr/rttr_enable.h>

#define JOB_SLICING_LOGSPACE "logspace"
#define JOB_SLICING_INVLOGSPACE "inv_logspace"
#define JOB_SLICING_LINSPACE "linspace"

#define CONFIG_FOLDER_NAME "config_files"
#define OUTPUT_FOLDER_NAME "sim_results"

struct ParamScheduler {
    ParamScheduler();
    int num_of_params;
    std::vector<std::map<std::string, std::string>> param_info_table;
    std::map<std::string, arma::vec> param_val_map;

    void load_param_info_table_from_json(std::vector<nlohmann::json> param_info_json_list);
    void load_param_val_from_folder(std::string folder);
    void process_prameter_vec_with_job_slicing_strategy(int job_id, int num_job_group, std::string job_slicing_strategy);
    std::string get_param_val_string_for_ith_param(int param_idx);
};

struct sim_prototypes {
    sim_prototypes();
    sim_prototypes(const sim_prototypes &s);
    std::map<gate_tag_type, Gate *> gate_prototype_map;
    std::map<hamiltonian_tag_type, Hamiltonian *> ctrl_hamiltonian_prototype_map;
    std::map<hamiltonian_tag_type, Noise_Hamiltonian *> noise_hamiltonian_prototype_map;
};

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

    nlohmann::json sim_configs;
    nlohmann::json gate_configs;
    nlohmann::json hamiltonian_configs;

    bool will_record_unitary;
    bool will_record_all_measurement;
    double step_size;
    int iterations;
    std::vector<symbolic_matrix> rho_inits;
    std::vector<symbolic_matrix> observables;
    ParamScheduler param_schedule;
    sim_prototypes simulation_prototypes;

    std::string result_exact_path;

public:
    QSimTask();
    ~QSimTask();
    virtual void load_sim_configs();
    virtual void load_gate_configs();
    virtual void load_hamiltonian_configs();

    virtual sim_prototypes* reload_prototypes_with_sweeping_parameter(int index);
    std::vector<arma::cx_cube> launch_solver(Sequence *seq, sim_prototypes *prototypes);
    void measument_solver();

    void preload() {
        load_sim_configs();
        load_hamiltonian_configs();
        load_gate_configs();
    }

    virtual void sweeping_task();

    arma::cx_cube* compile_time_dep_ctrl_hamiltonian(std::map<hamiltonian_tag_type, Hamiltonian *> hamiltonian_prototype_map, std::map<gate_tag_type, Gate *> gate_map ,Sequence seq);
    arma::cx_cube* compile_time_dep_noise_hamiltonian(std::map<hamiltonian_tag_type, Noise_Hamiltonian *> hamiltonian_prototype_map,int noise_idx, std::vector<double> random_start_pos_factor, int total_num_steps);
private:
    static nlohmann::json load_config_from_path(const std::string& path);
    void task_log(std::string message, int log_level);
};
