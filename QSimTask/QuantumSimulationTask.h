//
// Created by Rocky Su on 18/9/20.
//

#include <vector>
#include <stack>
#include <nlohmann/json.hpp>

#include "QSimCoreLib/Hamiltonian.h"
#include "QSimCoreLib/Gate.h"
#include "QSimCoreLib/VonNeumannSolver.h"

enum TaskStatus{
    Init,
    Preloaded,
    Finished,
    Recorded,
};

class QSimTask {
public:
    TaskStatus status;
    std::string config_file_folder;

    nlohmann::json sim_configs;
    nlohmann::json gate_configs;
    nlohmann::json hamiltonian_configs;

    std::map<gate_tag_type, Gate *> gate_prototype_map;
    std::map<hamiltonian_tag_type, Hamiltonian *> hamiltonian_prototype_map;
    std::vector<arma::cx_mat> rho_inits;
    std::vector<arma::cx_cube> rho_t_results;

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
    virtual void load_sequence();
//    virtual void load_task_configs();
//
//    virtual std::string get_task_description();
//    virtual void hamiltonian_define();
//    virtual void sim_parameter_define();
//    virtual void construct_initialStates();
//    virtual void construct_sequence();
//    virtual void construct_noise();
//    virtual void construct_measurement_info();
//    virtual void filter_analysis();
//    virtual void plotting();
//    virtual std::string record();
//    virtual void reset();


    void preload() {
        std::cout << "QSimTask: Loading simulation configs\n" << std::endl;
        load_sim_configs();
        std::cout << "QSimTask: Loading Hamiltonian prototypes\n" << std::endl;
        load_hamiltonian_configs();
        std::cout << "QSimTask: Loading Gate prototypes\n" << std::endl;
        load_gate_configs();
        std::cout << "QSimTask: Loading Sequence\n" << std::endl;
        load_sequence();
//        std::cout << task_brief_info() << "Start preload\n" << std::endl;
//        read_from_config_file();
//        std::cout << "Hamiltonian Loading\n" << std::endl;
//        hamiltonian_define();
//        sim_parameter_define();
//        load_solver();
//        std::cout << "Constructing Initial States\n" << std::endl;
//        construct_initialStates();
//        std::cout << "Constructing Seq\n" << std::endl;
//        construct_sequence();
//        std::cout << "Constructing Noise\n" << std::endl;
//        construct_noise();
//        std::cout << "Constructing Measurement Info\n" << std::endl;
//        construct_measurement_info();
//        std::cout << "Loading Seq to Solver\n" << std::endl;
//        load_sequence_to_solver();
//        std::cout << "Loading Noise to Solver\n" << std::endl;
//        load_noise_to_solver();
//        std::cout << "Loading Measurements to Solver\n" << std::endl;
//        load_measurements_to_solver();
//        status = Preloaded;
//        std::cout << task_brief_info() << "Preloaded!\n" << std::endl;
    }

private:
    static nlohmann::json load_config_from_path(const std::string& path);
};