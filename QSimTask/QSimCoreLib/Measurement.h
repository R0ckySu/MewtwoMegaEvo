//
// Created by Rocky Su on 7/10/20.
//
#include <armadillo>

typedef std::string observable_name_type;
typedef std::string init_state_name_type;
struct MeasurementError{
    std::string error_operator_symbol;
    arma::cx_mat error_operator;
    double errot_probability;
};

class MeasurementManager {
public:
    double step_size;
    MeasurementManager(std::vector<observable_name_type> observables_,std::vector<init_state_name_type> init_states_);
    std::vector<MeasurementError> errors;
    std::vector<observable_name_type> observables;
    std::vector<init_state_name_type> init_states;

    std::map<observable_name_type,std::map<init_state_name_type, arma::vec>> meas_result_set;
    arma::vec measurement_time_point_vec;

    void measure_from_density_mat_with_all_time_points(std::vector<arma::cx_cube> rho_multi);
    void measure_from_density_mat_with_time_points(std::vector<arma::cx_cube> rho_multi, arma::vec time_points);
    void save_result_to_folder(const std::string& path, const std::string& param_label);
private:
    std::vector<int> get_time_index(arma::vec time_points);
    void measure_density_mat_at_indices(std::vector<arma::cx_cube> rho_multi, std::vector<int> time_indices);
};