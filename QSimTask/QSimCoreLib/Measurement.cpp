//
// Created by Rocky Su on 7/10/20.
//

#include "Measurement.h"

MeasurementManager::MeasurementManager(std::vector<symbolic_matrix> observables_,
                                       std::vector<symbolic_matrix> init_states_) {
    observables = observables_;
    init_states = init_states_;
    measurement_time_point_vec = std::vector<double>();
    //Initialise the measurement results container
    for (int i = 0; i < observables_.size(); ++i) {
        observable_name_type O = observables_.at(i).symbol_name;
        std::map<init_state_name_type, arma::vec> res_rhos_map;
        for (int j = 0; j < init_states_.size(); ++j) {
            init_state_name_type rho_0 = init_states_.at(j).symbol_name;
            arma::vec res_temp = arma::vec();
            res_rhos_map.insert(std::make_pair(rho_0,res_temp));
        }
        meas_result_set.insert(std::make_pair(O,res_rhos_map));
    }
}

void MeasurementManager::measure_from_density_mat_with_time_points(std::vector<arma::cx_cube> rho_multi,
                                                                   arma::vec time_points) {
    std::vector<int> time_indices = get_time_index(time_points);
    measurement_time_point_vec = time_points;
    measure_density_mat_at_indices(rho_multi,time_indices);
}

void MeasurementManager::measure_from_density_mat_with_all_time_points(std::vector<arma::cx_cube> rho_multi) {
    int total_num_of_time_step = rho_multi.at(0).n_slices;

    arma::vec time_indices = arma::linspace(0,total_num_of_time_step-1,total_num_of_time_step);
    measure_density_mat_at_indices(rho_multi,arma::conv_to<std::vector<int>>::from(time_indices));
    measurement_time_point_vec = step_size * time_indices;
}

void MeasurementManager::measure_density_mat_at_indices(std::vector<arma::cx_cube> rho_multi,
                                                        std::vector<int > time_indices) {
    for (int i = 0; i < observables.size(); ++i) {
        observable_name_type O_symbol = observables.at(i).symbol_name;
        arma::cx_mat O = observables.at(i).mat;
        for (int j = 0; j < init_states.size(); ++j) {
            init_state_name_type rho_sym = init_states.at(j).symbol_name;
            arma::vec meas_temp = arma::vec(time_indices.size());
            auto rho_temp = rho_multi.at(j);
            #pragma omp parallel for default(none) shared(meas_temp,time_indices,O,rho_temp)
            for (int k = 0; k < time_indices.size(); ++k) {
                meas_temp.at(k) = arma::trace(arma::real(rho_temp.slice(time_indices.at(k)) * O));
            }
            meas_result_set[O_symbol][rho_sym] = meas_temp;
        }
    }
}

void MeasurementManager::save_result_to_folder(const std::string& path, const std::string& param_label) {
    //Storing data in hdf5 format
    for (const auto& O_entry: meas_result_set) {
        observable_name_type O_symbol = O_entry.first;
        for (const auto& rho_entry : O_entry.second) {
            init_state_name_type  rho_sym = rho_entry.first;
            std::string h5field_name = std::string(param_label).append("/").append(O_symbol).append("/").append(rho_sym);
            rho_entry.second.save(arma::hdf5_name(path,h5field_name,arma::hdf5_opts::append));
        }
    }

    std::string h5field_name = std::string(param_label).append("/").append("time_vec");
    measurement_time_point_vec.save(arma::hdf5_name(std::string(path),h5field_name,arma::hdf5_opts::append));

    //Clean up containers after storage;
    for (auto& O_entry: meas_result_set) {
        for (auto& rho_entry : O_entry.second) {
            arma::vec().swap(rho_entry.second);
        }
    }
    arma::vec().swap(measurement_time_point_vec);
}


std::vector<int> MeasurementManager::get_time_index(arma::vec time_points) {
    std::vector<int> time_index = std::vector<int>(time_points.n_elem);
    for (int i = 0; i < time_points.n_elem; ++i) {
        time_index.at(i) = round(time_points.at(i)/step_size);
    }
    return time_index;
}


