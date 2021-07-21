//
// Created by Rocky Su on 16/9/20.
//

#include <string>
#include "Hamiltonian.h"
#include "Utils.h"
#include <rttr/registration.h>

RTTR_REGISTRATION{
    rttr::registration::class_<Hamiltonian>("Hamiltonian")
            .property("amplitude",&Hamiltonian::amplitude)
            .property("h_mat",&Hamiltonian::h_mat);

    rttr::registration::class_<Gated_Hamiltonian>("Gated_Hamiltonian")
            .property("amplitude",&Gated_Hamiltonian::amplitude)
            .property("falling_time",&Gated_Hamiltonian::falling_time)
            .property("rising_time",&Gated_Hamiltonian::rising_time);

    rttr::registration::class_<MW_Hamiltonian>("MW_Hamiltonian")
            .property("amplitude",&MW_Hamiltonian::amplitude)
            .property("freq",&MW_Hamiltonian::freq)
            .property("phase",&MW_Hamiltonian::phase);

    rttr::registration::class_<Noise_Hamiltonian>("Noise_Hamiltonian")
            .property("shift_time",&Noise_Hamiltonian::shift_time)
            .property("amplitude",&MW_Hamiltonian::amplitude);
};

/*********************************************Hamiltonian Classes******************************************************/

void Hamiltonian::load_ext_waveform(int param_index) {
    if (external_waveform_path.length() > 0) {
        std::string datapath_index = std::string(external_waveform_path);
        int sharp_pos = datapath_index.find_first_of('#');
        datapath_index.insert(sharp_pos+1,std::to_string(param_index));
        wave_form.load(datapath_index,arma::csv_ascii);
        wave_form = wave_form * amplitude;
        std::cout << "Hamiltonian: Loaded external waveform:" << datapath_index << std::endl;
    }
}

Hamiltonian::Hamiltonian() {
    step_size = 1;
    num_of_steps = 1;
    amplitude = 0.0;
    h_mat = symbolic_matrix();
    switching_signal = arma::vec().fill(0);
    wave_form = arma::cx_vec().fill(0);
    times_vec = arma::vec().fill(0);
    external_waveform_path="";
}

Hamiltonian::Hamiltonian(const Hamiltonian &h) {
    tag = h.tag;
    amplitude = h.amplitude;
    h_mat = h.h_mat;
    step_size = h.step_size;
    num_of_steps = h.num_of_steps;
    external_waveform_path = h.external_waveform_path;
}

Hamiltonian::Hamiltonian(nlohmann::json h_config, std::string config_path) {
    tag = h_config["tag"];
    amplitude = h_config["amplitude"];
    std::string h_string = h_config["h_pauli_mat"];
    h_mat.load_from_symbol(h_string,config_path);
    external_waveform_path = h_config["waveform_path"];
    std::cout << tag << ":\n" << h_mat.mat << std::endl;
}

Hamiltonian::~Hamiltonian() {
    arma::cx_vec().swap(wave_form);
    arma::vec().swap(times_vec);
    arma::vec().swap(switching_signal);
}

void Hamiltonian::load_waveform() {
    times_vec = step_size * arma::linspace(0,num_of_steps+1,num_of_steps+1);
    wave_form = arma::cx_vec(num_of_steps).fill(0);
}

void Hamiltonian::fetch_H(arma::cx_cube *H0) {}

std::string Hamiltonian::description() {
    return "Hamiltonian";
}

void Hamiltonian::clean_up_on_reload() {
//    step_size = 1;
//    num_of_steps = 0;
    wave_form.fill(0);
    switching_signal.fill(0);
    times_vec.fill(0);

    arma::vec().swap(times_vec);
    arma::vec().swap(switching_signal);
    arma::cx_vec().swap(wave_form);
}

void Hamiltonian::add_signal(arma::vec _sig) {
    if (switching_signal.size() == 0) {
        switching_signal = _sig;
    } else if (_sig.size() == switching_signal.size()) {
        switching_signal += _sig;
    } else {
        std::cout << "Hamiltonian:" << tag << " Mismatched signal length!" << std::endl;
        std::cout << "Expect length:" << switching_signal.n_elem << " received:" << _sig.n_elem << std::endl;
    }
}

Hamiltonian *Hamiltonian::clone() {
    return new Hamiltonian(*this);
}

/**********************************************************************************************************************/

Static_Hamiltonian *Static_Hamiltonian::clone() {
    return new Static_Hamiltonian(*this);
}

Static_Hamiltonian::Static_Hamiltonian(nlohmann::json h_config, std::string config_path) : Hamiltonian(h_config, config_path) {}

void Static_Hamiltonian::fetch_H(arma::cx_cube *H0) {
    for (int i = 0; i < H0->n_slices; ++i) {
        H0->slice(i) += step_size * h_mat.mat * wave_form(i);
    }
}

void Static_Hamiltonian::load_waveform() {
    Hamiltonian::load_waveform();
    wave_form.fill(amplitude * M_PI);
}

/**********************************************************************************************************************/

Gated_Hamiltonian::Gated_Hamiltonian() {
    rising_time = 0;
    falling_time = 0;
}

Gated_Hamiltonian::Gated_Hamiltonian(nlohmann::json h_config, std::string config_path) : Hamiltonian(h_config,config_path) {
    rising_time = h_config["rising_time"];
    falling_time = h_config["falling_time"];
}

Gated_Hamiltonian::Gated_Hamiltonian(const Hamiltonian &h, const Gated_Hamiltonian &a): Hamiltonian(h) {
    rising_time = a.rising_time;
    falling_time = a.falling_time;
}

Gated_Hamiltonian *Gated_Hamiltonian::clone() {
    return new Gated_Hamiltonian(*this);
}

void Gated_Hamiltonian::load_waveform() {
    Hamiltonian::load_waveform();
    if (rising_time != 0.0 && falling_time != 0.0) {
        int rising_sig_length = floor(rising_time/step_size) + 1;
        int falling_sig_length = floor(falling_time/step_size) + 1;

        arma::vec rising_sig = arma::linspace(0,1,rising_sig_length);
        arma::vec falling_sig = arma::linspace(1,0,falling_sig_length);

        if (switching_signal.size() != 0) {
            for (int i = 1; i < switching_signal.size() - 1; ++i) {
                if ((switching_signal.at(i) == 1.0) && (switching_signal.at(i - 1) == 0.0)) {
                    //Rising edge condition
                    switching_signal.subvec(i-1, i + rising_sig_length - 2) = rising_sig;
                }

                if ((switching_signal.at(i - 1) == 1.0) && (switching_signal.at(i) == 0.0)) {
                    //Falling edge condition
                    switching_signal.subvec(i - 1, i + falling_sig_length - 2) = falling_sig;
                }
            }
        }
    }
}

void Gated_Hamiltonian::fetch_H(arma::cx_cube *H0) {
    Hamiltonian::fetch_H(H0);
}

void Gated_Hamiltonian::clean_up_on_reload() {
    Hamiltonian::clean_up_on_reload();
}

std::string Gated_Hamiltonian::description() {
    return Hamiltonian::description();
}

void Gated_Hamiltonian::add_signal(arma::vec _sig) {
    Hamiltonian::add_signal(_sig);
}

/**********************************************************************************************************************/

MW_Hamiltonian::MW_Hamiltonian() {
    freq = 0;
    phase = 0;
}

MW_Hamiltonian::MW_Hamiltonian(const Gated_Hamiltonian &g, const MW_Hamiltonian &m): Gated_Hamiltonian(g) {
    freq = m.freq;
    phase = m.phase;
}

MW_Hamiltonian *MW_Hamiltonian::clone() {
    return new MW_Hamiltonian(*this);
}

MW_Hamiltonian::MW_Hamiltonian(nlohmann::json h_config, std::string config_path) : Gated_Hamiltonian(h_config, config_path)  {
    freq = h_config["freq"];
    phase = h_config["phase"];
}

void MW_Hamiltonian::load_waveform() {
    Gated_Hamiltonian::load_waveform();
    std::cout << "MW_Hamiltonian: Loading waveform" << std::endl;
    if(switching_signal.size()) {
        const arma::cx_double j = arma::cx_double(0,1);
        if(freq == 0.){
            for (int i = 0; i < switching_signal.size(); ++i){
                if (switching_signal.at(i) != 0.0) {
                    wave_form[i] = M_PI*step_size*amplitude*switching_signal.at(i)*std::exp(j*phase);
                }
            }
        }
        else {
            std::cout << "Microwave:Non RF: freq=" << freq << std::endl;
            for (int i = 0; i < switching_signal.size(); ++i) {
                if (switching_signal.at(i) != 0.0) {
                    wave_form[i] = M_PI*step_size*amplitude*switching_signal.at(i)*std::exp(j*(times_vec[i]*freq*M_PI + phase));
//                    wave_form[i] = amplitude * (get_amplitude(times_vec[i]) + get_amplitude(times_vec[i + 1])) / 2 *
//                                   std::exp(j * phase) / (j * freq * M_PI * 2.) * (
//                                           std::exp(j * freq * 2. * M_PI * (times_vec[i + 1]))
//                                           - std::exp(j * freq * 2. * M_PI * (times_vec[i])));
                }
            }
        }
//        std::cout << "waveform:" << wave_form << std::endl;
    }

//        pulse_data_conj = arma::conj(pulse_data);
}

void MW_Hamiltonian::fetch_H(arma::cx_cube *H0) {
    Gated_Hamiltonian::fetch_H(H0);
    std::cout << "MW_Hamiltonian: Fetching Hamiltonian" << std::endl;
    arma::cx_vec wave_form_conj = arma::conj(wave_form);
    arma::cx_mat matrix_element_up = arma::trimatu(h_mat.mat);
    arma::cx_mat matrix_element_down = arma::trimatu(h_mat.mat,1).t();

    for (int i = 0; i < H0->n_slices; ++i) {
        H0->slice(i) += matrix_element_up*wave_form(i) + matrix_element_down*wave_form_conj(i);
    }
}

std::string MW_Hamiltonian::description() {
    return "Microwave  Hamiltonian";
}

void MW_Hamiltonian::clean_up_on_reload() {
    Gated_Hamiltonian::clean_up_on_reload();
}

void MW_Hamiltonian::load_and_fetch_H_with_dynamic_frame(arma::cx_cube *H0) {
    const arma::cx_double j = arma::cx_double(0,1);

    // Generating Waveform for each transition section
    for (int i = 0; i < frame_trans_time_pos.n_elem; ++i) {

        // Cancelling out the freq of the rotating frame
        arma::mat mw_freq_mat = arma::zeros(h_mat.mat.n_cols,h_mat.mat.n_rows);
        std::cout << "--------------------\n MW_Hamiltonian: frame trans pos:" << frame_trans_time_pos.at(i) << std::endl;
        std::cout << "MW_Hamiltonian: h_mat:\n" << h_mat_under_time_dep_frame.slice(i) << std::endl;
        arma::uvec non0inH = arma::find(arma::abs(arma::trimatu(h_mat_under_time_dep_frame.slice(i))) > 0.01);
        mw_freq_mat.elem(non0inH) += 1;
        mw_freq_mat = mw_freq_mat * freq;
        arma::mat freq_diff_to_RF = freq_mask_time_dep.slice(i)- mw_freq_mat;
        freq_diff_to_RF.elem(find(arma::abs(freq_diff_to_RF) > 1/step_size)) -= freq_diff_to_RF.elem(find(arma::abs(freq_diff_to_RF) > 1/step_size));
        std::cout << "MW_Hamiltonian: MW freq mask mat:\n" << freq_mask_time_dep.slice(i) << std::endl;
        std::cout << "MW_Hamiltonian: MW freq diff mat:\n" << freq_diff_to_RF << std::endl;

//        arma::uvec non0_freq_pos = arma::find(arma::abs(arma::trimatu(freq_diff_to_RF))>0);
//        std::cout << "MW_Hamiltonian: MW freq diff mat:\n" << non0_freq_pos << std::endl;

        uint start_pos = frame_trans_time_pos.at(i);
        uint end_pos = start_pos;
        if (i == (frame_trans_time_pos.n_elem-1)) {
            end_pos = H0->n_slices-1;
        } else {
            end_pos = frame_trans_time_pos.at(i+1)-1;
        }
//        wave_form[i] = M_PI*step_size*amplitude*switching_signal.at(i)*std::exp(j*phase);

        for (int non0_elem_idx = 0; non0_elem_idx < non0inH.n_elem; ++non0_elem_idx) {
            uint col_idx = floor(non0inH.at(non0_elem_idx)/h_mat.mat.n_rows);
            uint row_idx = non0inH.at(non0_elem_idx) - col_idx*h_mat.mat.n_rows;
            double freq_rf =  freq_diff_to_RF.at(row_idx,col_idx);
            std::cout << "MW_Hamiltonian: MW oscillator at freq:" << freq_rf  << " start:" << start_pos << " end:" << end_pos << std::endl;

            for (int time_idx = start_pos; time_idx <= end_pos; ++time_idx) {
                arma::cx_double sig = M_PI * h_mat_under_time_dep_frame(row_idx,col_idx,i) * step_size * amplitude*switching_signal.at(time_idx)*std::exp(j*(M_PI * step_size * time_idx *freq_rf + phase));
                H0->at(row_idx, col_idx, time_idx) += sig;
                H0->at(col_idx, row_idx, time_idx) += std::conj(sig);
            }
        }
    }
}

/**********************************************************************************************************************/

AWG_Hamiltonian::AWG_Hamiltonian() {}

AWG_Hamiltonian::AWG_Hamiltonian(nlohmann::json h_config, std::string config_path) : Gated_Hamiltonian(h_config,config_path) {}

AWG_Hamiltonian::AWG_Hamiltonian(const Gated_Hamiltonian &g, const AWG_Hamiltonian &a): Gated_Hamiltonian(g) {

}

void AWG_Hamiltonian::load_waveform() {
    Gated_Hamiltonian::load_waveform();
    wave_form = arma::cx_vec(  M_PI*step_size * amplitude * switching_signal, arma::zeros(switching_signal.size()));
//    std::cout << "waveform of:"<< tag << "\n" << "amp:" << amplitude << "step size" << step_size << std::endl;
}

void AWG_Hamiltonian::fetch_H(arma::cx_cube *H0) {
    Gated_Hamiltonian::fetch_H(H0);
    for (int i = 0; i < H0->n_slices; ++i) {
        H0->slice(i) += h_mat.mat * wave_form(i);
    }
}

std::string AWG_Hamiltonian::description() {
    return "AWG Hamiltonian";
}

AWG_Hamiltonian *AWG_Hamiltonian::clone() {
    return new AWG_Hamiltonian(*this);
}

void AWG_Hamiltonian::clean_up_on_reload() {
    Gated_Hamiltonian::clean_up_on_reload();
}

/**********************************************************************************************************************/
Noise_Hamiltonian::Noise_Hamiltonian(): Hamiltonian() {}

Noise_Hamiltonian::Noise_Hamiltonian(const Hamiltonian &h, const Noise_Hamiltonian &m): Hamiltonian(h) {
    shift_time = m.shift_time;
    randomStartPosFactor = m.randomStartPosFactor;
}

Noise_Hamiltonian::Noise_Hamiltonian(nlohmann::json noise_config, std::string config_path) : Hamiltonian(noise_config, config_path)  {
    shift_time = noise_config["lag_time"];
}

void Noise_Hamiltonian::fetch_H(arma::cx_cube *H0) {
    Hamiltonian::fetch_H(H0);
    for (int i = 0; i < H0->n_slices; ++i) {
        H0->slice(i) += h_mat.mat*wave_form(i)*step_size;
    }
}

void Noise_Hamiltonian::load_ext_waveform(int param_index) {
    Hamiltonian::load_ext_waveform(param_index);
    int raw_wave_total_length = wave_form.size();

    int shift_steps = 0;
    if (shift_time != 0) {
        shift_steps = floor(shift_time/step_size);
    }

    int residual_steps = raw_wave_total_length - num_of_steps - shift_steps;
    if (residual_steps > 0) {
        shift_steps += floor(residual_steps * randomStartPosFactor);
    } else {
        std::cout << "Noise_Hamiltonian Error: Insufficient length of raw data! Expect:" << num_of_steps + shift_steps << " Provided:" << raw_wave_total_length << std::endl;
    }

    wave_form = wave_form.subvec(shift_steps,shift_steps+num_of_steps);
    std::cout << "Noise_Hamiltonian: shifted by" << shift_steps << std::endl;
}

std::string Noise_Hamiltonian::description() {
    return "Noise Hamiltonian";
}

Noise_Hamiltonian *Noise_Hamiltonian::clone() {
    return new Noise_Hamiltonian(*this);
}

Noise_Hamiltonian::~Noise_Hamiltonian() = default;