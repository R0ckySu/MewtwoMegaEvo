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
            .property("waveform_path", &Hamiltonian::external_waveform_path)
            .property("h_mat",&Hamiltonian::h_mat);

    rttr::registration::class_<Gated_Hamiltonian>("Gated_Hamiltonian")
            .property("falling_time",&Gated_Hamiltonian::falling_time)
            .property("rising_time",&Gated_Hamiltonian::rising_time);

    rttr::registration::class_<MW_Hamiltonian>("MW_Hamiltonian")
            .property("falling_time",&MW_Hamiltonian::falling_time)
            .property("rising_time",&MW_Hamiltonian::rising_time)
            .property("amplitude",&MW_Hamiltonian::amplitude)
            .property("freq",&MW_Hamiltonian::freq)
            .property("phase",&MW_Hamiltonian::phase);

    rttr::registration::class_<AWG_Hamiltonian>("AWG_Hamiltonian")
            .property("falling_time",&AWG_Hamiltonian::falling_time)
            .property("rising_time",&AWG_Hamiltonian::rising_time)
            .property("amplitude",&MW_Hamiltonian::amplitude);

    rttr::registration::class_<Noise_Hamiltonian>("Noise_Hamiltonian")
            .property("shift_time",&Noise_Hamiltonian::shift_time);
};

/*********************************************Hamiltonian Classes******************************************************/

void Hamiltonian::load_ext_waveform(int param_index) {
    if (external_waveform_path.length() > 0) {
        std::string datapath_index = std::string(external_waveform_path);
        int sharp_pos = datapath_index.find_first_of('#');
        datapath_index.insert(sharp_pos+1,std::to_string(param_index));
        wave_form = (*ext_sig_cache).load_from_cache(datapath_index);
//        wave_form.load(datapath_index,arma::csv_ascii);
        wave_form = wave_form * amplitude;
        std::cout << "Hamiltonian:"<< tag << " Loaded external waveform:" << datapath_index << std::endl;
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
            for (int i = 0; i < switching_signal.size(); ++i){
                if (switching_signal.at(i) != 0.0) {
                    wave_form[i] = M_PI*step_size*amplitude*switching_signal.at(i)*std::exp(j*(times_vec[i]*freq*2.*M_PI + phase));
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

MW_Hamiltonian::~MW_Hamiltonian() {
    arma::cx_vec().swap(wave_form);
    arma::vec().swap(times_vec);
    arma::vec().swap(switching_signal);
}

/**********************************************************************************************************************/

AWG_Hamiltonian::AWG_Hamiltonian() {}

AWG_Hamiltonian::AWG_Hamiltonian(nlohmann::json h_config, std::string config_path) : Gated_Hamiltonian(h_config,config_path) {}

AWG_Hamiltonian::AWG_Hamiltonian(const Gated_Hamiltonian &g, const AWG_Hamiltonian &a): Gated_Hamiltonian(g) {

}

void AWG_Hamiltonian::load_waveform() {
    Gated_Hamiltonian::load_waveform();
    wave_form = arma::cx_vec(M_PI * amplitude * switching_signal * step_size, arma::zeros(switching_signal.size()));
//    std::cout << "waveform of:"<< tag << "\n" << wave_form << std::endl;
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
    external_waveform_path = m.external_waveform_path;
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

    int fixed_shift_steps = 0;
    if (shift_time != 0) {
        fixed_shift_steps = floor(shift_time/step_size);
    }

    int rand_shift_steps = 0;
    int residual_steps = raw_wave_total_length - num_of_steps;
    if (residual_steps > 0) {
        rand_shift_steps += floor(residual_steps * randomStartPosFactor);
    } else {
        std::cout << "Noise_Hamiltonian Error: Insufficient length of raw data! Expect:" << num_of_steps + rand_shift_steps << " Provided:" << raw_wave_total_length << std::endl;
    }

    int total_shift_steps = rand_shift_steps + fixed_shift_steps;
    if (total_shift_steps > raw_wave_total_length) {
        std::cout << "Noise_Hamiltonian Error: Insufficient length of raw data when have fixed shift! Expect:" << total_shift_steps << " Provided:" << raw_wave_total_length << std::endl;
    }

    wave_form = wave_form.subvec(total_shift_steps,total_shift_steps+num_of_steps);
    std::cout << "Noise_Hamiltonian:"<< tag << " shifted by" << total_shift_steps << std::endl;
}

std::string Noise_Hamiltonian::description() {
    return "Noise Hamiltonian";
}

Noise_Hamiltonian *Noise_Hamiltonian::clone() {
    return new Noise_Hamiltonian(*this);
}

Noise_Hamiltonian::~Noise_Hamiltonian() {
    arma::cx_vec().swap(wave_form);
    arma::vec().swap(times_vec);
    arma::vec().swap(switching_signal);
}