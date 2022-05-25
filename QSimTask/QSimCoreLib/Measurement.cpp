//
// Created by Rocky Su on 7/10/20.
//

#include "Measurement.h"

#define MEAS_MARKER_FIELD_NAME "meas_marker"
#define MEAS_ALL_FIELD_NAME "meas_all"
#define DEN_MAT_MARKER_FIELD_NAME "rho_marker"
#define DEN_MAT_ALL_FIELD_NAME "rho_all"

MeasurementManager::MeasurementManager(std::vector<symbolic_matrix> observables_,
                                       std::vector<symbolic_matrix> init_states_) {
    observables = observables_;
    init_states = init_states_;
    measurement_time_point_vec = std::vector<double>();
    //Initialise the measurement results container
    for (int i = 0; i < observables_.size(); ++i) {
        observable_name_type O = observables_.at(i).symbol_name;
        auto res_rhos_map = std::map<init_state_name_type, arma::vec>();
        for (int j = 0; j < init_states_.size(); ++j) {
            init_state_name_type rho_0 = init_states_.at(j).symbol_name;
            arma::vec res_temp = arma::vec();
            res_rhos_map.insert(std::make_pair(rho_0,res_temp));
        }
    }
}

void MeasurementManager::measure_from_density_mat_with_time_points(std::vector<arma::cx_cube> rho_multi,
                                                                   arma::vec time_points) {
    if (will_record_density_mat) {
        rho_multi_t = rho_multi;
    }
    std::vector<uint> time_indices_marker = get_time_index(time_points);
    measurement_time_point_vec = time_points;
    meas_marker_result = measure_density_mat_at_indices(rho_multi,time_indices_marker);

    if (will_record_all_time_points_meas) {
        int total_num_of_time_step = rho_multi.at(0).n_slices;
        arma::vec time_indices_all = arma::linspace(0,total_num_of_time_step-1,total_num_of_time_step);
        meas_all_result =  measure_density_mat_at_indices(rho_multi,arma::conv_to<std::vector<uint>>::from(time_indices_all));
    }
}

void MeasurementManager::measure_from_density_mat_with_all_time_points(std::vector<arma::cx_cube> rho_multi) {
    int total_num_of_time_step = rho_multi.at(0).n_slices;

    arma::vec time_indices = arma::linspace(0,total_num_of_time_step-1,total_num_of_time_step);
    measure_density_mat_at_indices(rho_multi,arma::conv_to<std::vector<uint>>::from(time_indices));
    measurement_time_point_vec = step_size * time_indices;
}

std::map<observable_name_type,std::map<init_state_name_type, arma::vec>> * MeasurementManager::measure_density_mat_at_indices(std::vector<arma::cx_cube> rho_multi,
                                                        std::vector<uint > time_indices) {
    std::cout << "Measuring density mat" << std::endl;
    auto meas_result_temp = create_new_meas_result_container();
    for (int i = 0; i < observables.size(); ++i) {
        observable_name_type O_symbol = observables.at(i).symbol_name;
        arma::cx_mat O = observables.at(i).mat;
        for (int j = 0; j < init_states.size(); ++j) {
            init_state_name_type rho_sym = init_states.at(j).symbol_name;
            std::cout << "Measuring O:" << O_symbol << " rho:" << rho_sym << std::endl;

            arma::vec meas_temp = arma::vec(time_indices.size());
            auto rho_temp = rho_multi.at(j);
            #pragma omp parallel for default(none) shared(meas_temp,time_indices,O,rho_temp)
            for (int k = 0; k < time_indices.size(); ++k) {
                meas_temp.at(k) = arma::trace(arma::real(rho_temp.slice(time_indices.at(k)) * O));
            }
            (*meas_result_temp)[O_symbol][rho_sym] = meas_temp;
        }
    }
    return meas_result_temp;
}

void MeasurementManager::save_result_to_h5(const std::string& path,  const int param_idx, const std::map<std::string, std::any> param_info) {
    //Storing data in hdf5 format
    std::string param_idx_str = std::string("/#").append(std::to_string(param_idx));

    for (const auto& O_entry: *meas_marker_result) {
        observable_name_type O_symbol = O_entry.first;
        for (const auto& rho_entry : O_entry.second) {
            init_state_name_type  rho_sym = rho_entry.first;
            std::string h5field_name = std::string(MEAS_MARKER_FIELD_NAME).append(param_idx_str).append("/").append(O_symbol).append("/").append(rho_sym);
            rho_entry.second.save(arma::hdf5_name(path,h5field_name,arma::hdf5_opts::append));
            //Release Mem after saving
            arma::vec().swap((*meas_marker_result)[O_symbol][rho_sym]);
            if (will_record_all_time_points_meas) {
                std::string meas_all_h5field_name = std::string(MEAS_ALL_FIELD_NAME).append(param_idx_str).append("/").append(O_symbol).append("/").append(rho_sym);
                (*meas_all_result)[O_symbol][rho_sym].save(arma::hdf5_name(path,meas_all_h5field_name,arma::hdf5_opts::append));
                //Release Mem after saving
                arma::vec().swap((*meas_all_result)[O_symbol][rho_sym]);
            }
        }
    }
    std::string h5field_name = std::string(MEAS_MARKER_FIELD_NAME).append(param_idx_str).append("/").append("time_vec");
    measurement_time_point_vec.save(arma::hdf5_name(std::string(path),h5field_name,arma::hdf5_opts::append));

    std::vector<uint> time_indices_marker = get_time_index(measurement_time_point_vec);
    arma::uvec idx_arma = arma::conv_to<arma::uvec>::from(time_indices_marker);
    if (will_record_density_mat) {
        std::string dm_marker_field_name = std::string(DEN_MAT_MARKER_FIELD_NAME).append(param_idx_str).append("/");
        std::string dm_all_field_name = std::string(DEN_MAT_ALL_FIELD_NAME).append(param_idx_str).append("/");
        for (int j = 0; j < (rho_multi_t).size(); ++j) {
            std::string sub_field_name = std::string(dm_marker_field_name);
            sub_field_name.append(init_states.at(j).symbol_name);
//            std::string sub_field_name = dm_marker_field_name.append(init_states.at(j).symbol_name);
            arma::cx_cube rho_markers = (rho_multi_t).at(j).slices(idx_arma);
            rho_markers.save(arma::hdf5_name(path,sub_field_name,arma::hdf5_opts::append));
            if (will_record_all_time_points_meas) {
                std::string all_sub_field_name = dm_all_field_name.append(init_states.at(j).symbol_name);
                (rho_multi_t).at(j).save(arma::hdf5_name(path,all_sub_field_name,arma::hdf5_opts::append));
            }
            //Release Mem after saving
//            arma::cx_cube().swap((rho_multi_t).at(j));
        }
        std::string dm_h5field_name = std::string(DEN_MAT_MARKER_FIELD_NAME).append(param_idx_str).append("/").append("time_vec");
        measurement_time_point_vec.save(arma::hdf5_name(std::string(path),dm_h5field_name,arma::hdf5_opts::append));
    }

    //Release Mem after saving
    arma::vec().swap(measurement_time_point_vec);
}

void MeasurementManager::save_result_to_folder(const std::string& path, const std::string& param_label) {
    //Storing data in hdf5 format
    for (const auto& O_entry: *meas_marker_result) {
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
    for (auto& O_entry: *meas_marker_result) {
        for (auto& rho_entry : O_entry.second) {
            arma::vec().swap(rho_entry.second);
        }
    }
    arma::vec().swap(measurement_time_point_vec);
}


std::vector<uint> MeasurementManager::get_time_index(arma::vec time_points) {
    std::vector<uint> time_index = std::vector<uint>(time_points.n_elem);
    for (int i = 0; i < time_points.n_elem; ++i) {
        time_index.at(i) = round(time_points.at(i)/step_size);
    }
    return time_index;
}

std::map<observable_name_type, std::map<init_state_name_type, arma::vec>> *
MeasurementManager::create_new_meas_result_container() {
    std::map<observable_name_type,std::map<init_state_name_type, arma::vec>> * meas_result = new std::map<observable_name_type,std::map<init_state_name_type, arma::vec>>;
    //Initialise the measurement results container
    for (int i = 0; i < observables.size(); ++i) {
        observable_name_type O = observables.at(i).symbol_name;
        auto res_rhos_map = std::map<init_state_name_type, arma::vec>();
        for (int j = 0; j < init_states.size(); ++j) {
            init_state_name_type rho_0 = init_states.at(j).symbol_name;
            arma::vec res_temp = arma::vec();
            res_rhos_map.insert(std::make_pair(rho_0,res_temp));
        }
        (*meas_result).insert(std::make_pair(O, res_rhos_map));
    }
    return meas_result;
}



