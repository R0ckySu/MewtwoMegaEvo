//
// Created by Rocky Su on 7/10/20.
//
#include <armadillo>
#include "Utils.h"
#include <any>
#include <iostream>

typedef std::string observable_name_type;
typedef std::string init_state_name_type;
struct MeasurementError{
    std::string error_operator_symbol;
    arma::cx_mat error_operator;
    double errot_probability;
};

class MeasurementManager {
public:
    bool will_record_all_time_points_meas = true;
    bool will_record_density_mat = false;

    double step_size;
    MeasurementManager(std::vector<symbolic_matrix> observables_,std::vector<symbolic_matrix> init_states_);
    std::vector<MeasurementError> errors;
    std::vector<symbolic_matrix> observables;
    std::vector<symbolic_matrix> init_states;

    std::map<observable_name_type,std::map<init_state_name_type, arma::vec>> * meas_marker_result;
    std::map<observable_name_type,std::map<init_state_name_type, arma::vec>> * meas_all_result;
    std::vector<arma::cx_cube> rho_multi_t;
    arma::vec measurement_time_point_vec;

    void measure_from_density_mat_with_all_time_points(std::vector<arma::cx_cube> rho_multi);
    void measure_from_density_mat_with_time_points(std::vector<arma::cx_cube> rho_multi, arma::vec time_points);
    void save_result_to_folder(const std::string& path, const std::string& param_label);
    void save_result_to_h5(const std::string& path, const int param_idx, const std::map<std::string, std::any> param_info);
private:
    std::vector<int> get_time_index(arma::vec time_points);
    std::map<observable_name_type,std::map<init_state_name_type, arma::vec>> * measure_density_mat_at_indices(std::vector<arma::cx_cube> rho_multi, std::vector<int> time_indices);
    std::map<observable_name_type,std::map<init_state_name_type, arma::vec>> * create_new_meas_result_container();
};