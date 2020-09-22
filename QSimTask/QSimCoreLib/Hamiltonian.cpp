//
// Created by Rocky Su on 16/9/20.
//

#include "Hamiltonian.h"
#include "Utils.h"

/*********************************************Hamiltonian Classes******************************************************/

void Hamiltonian::load_ext_waveform(std::string datapath) {

}

Hamiltonian::Hamiltonian() {
    step_size = 1;
    num_of_steps = 1;
    amplitude = 0.0;
    h_mat = arma::cx_mat().fill(0);
    wave_form = arma::cx_vec();
    times_vec = arma::vec();
    external_waveform_path="";
}

Hamiltonian::Hamiltonian(const Hamiltonian &h) {
    amplitude = h.amplitude;
    h_mat = h.h_mat;
    step_size = h.step_size;
    num_of_steps = h.num_of_steps;
}

Hamiltonian::Hamiltonian(nlohmann::json h_config) {
    amplitude = h_config["amplitude"];

    std::string h_string = h_config["h_pauli_mat"];
    std::vector<std::string> h_str_splitted = str_split(h_string,':');
    if (h_str_splitted.at(0) == "symbol") {
        h_mat = qmt::spinorDecoder(h_str_splitted.at(1));
    } else if (h_str_splitted.at(0) == "path") {
        h_mat = arma::cx_mat().load(h_str_splitted.at(1),arma::csv_ascii);
    }

    external_waveform_path = h_config["waveform_path"];;
}

Hamiltonian::~Hamiltonian() {
//    arma::cx_mat().swap(h_mat);
//    arma::cx_vec().swap(wave_form);
//    arma::vec().swap(times_vec);
//    arma::vec().swap(switching_signal);
}

void Hamiltonian::load_waveform() {
    times_vec = step_size * arma::linspace(0,num_of_steps+1,num_of_steps+1);
    wave_form = arma::cx_vec(num_of_steps);

    if (external_waveform_path.length() > 0) {
        wave_form.load(external_waveform_path,arma::hdf5_binary);
    }
}

void Hamiltonian::fetch_H(arma::cx_cube *H0) {}

std::string Hamiltonian::description() {
    return "Hamiltonian";
}

/**********************************************************************************************************************/
Static_Hamiltonian::Static_Hamiltonian(nlohmann::json h_config) : Hamiltonian(h_config) {}

/**********************************************************************************************************************/

MW_Hamiltonian::MW_Hamiltonian() {
    freq = 0;
    phase = 0;
}

MW_Hamiltonian::MW_Hamiltonian(const Hamiltonian &h, const MW_Hamiltonian &m): Hamiltonian(h) {
    freq = m.freq;
    phase = m.phase;
}

MW_Hamiltonian::MW_Hamiltonian(nlohmann::json h_config) : Hamiltonian(h_config) {
    freq = h_config["freq"];
    phase = h_config["phase"];
}

//MW_Hamiltonian::~MW_Hamiltonian() {
////    Hamiltonian::~Hamiltonian();
//}

void MW_Hamiltonian::load_waveform() {
    Hamiltonian::load_waveform();

    if(switching_signal.size() == num_of_steps) {
        const arma::cx_double j = arma::cx_double(0,1);
        if(freq == 0.){
            for (int i = 0; i < switching_signal.size(); ++i){
                if (switching_signal.at(i) == 1.0) {
                    wave_form[i] = step_size*amplitude*(get_amplitude(times_vec[i]) + get_amplitude(times_vec[i+1]))/2*std::exp(j*phase);
                }
            }
        }
        else{
            for (int i = 0; i < switching_signal.size(); ++i){
                if (switching_signal.at(i) == 1.0) {
                    wave_form[i] = amplitude * (get_amplitude(times_vec[i]) + get_amplitude(times_vec[i + 1])) / 2 *
                                   std::exp(j * phase) / (j * freq * M_PI * 2.) * (
                                           std::exp(j * freq * 2. * M_PI * (times_vec[i + 1]))
                                           - std::exp(j * freq * 2. * M_PI * (times_vec[i])));
                }
            }
        }
    }

//        pulse_data_conj = arma::conj(pulse_data);
}

double MW_Hamiltonian::get_amplitude(double time) const{
    if (modulation == undef){
        return 1;
    }
    if (modulation == gaussian) {
        return std::exp(-(time - gaussian_mod_param.mu)* (time - gaussian_mod_param.mu)/(2 * gaussian_mod_param.sigma * gaussian_mod_param.sigma));
    }
    return 0;
}

void MW_Hamiltonian::fetch_H(arma::cx_cube *H0) {
    Hamiltonian::fetch_H(H0);
    arma::cx_vec wave_form_conj = arma::conj(wave_form);
    arma::cx_mat matrix_element_up = arma::trimatu(h_mat);
    arma::cx_mat matrix_element_down = arma::trimatu(h_mat,1).t();

    for (int i = 0; i < wave_form.size(); ++i) {
        H0->slice(i) += matrix_element_up*wave_form(i) + matrix_element_down*wave_form_conj(i);
    }
}

std::string MW_Hamiltonian::description() {
    return "Microwave Hamiltonian";
}

/**********************************************************************************************************************/
Noise_Hamiltonian::Noise_Hamiltonian(): Hamiltonian() {

}

Noise_Hamiltonian::Noise_Hamiltonian(nlohmann::json noise_config) : Hamiltonian(noise_config) {

}

Noise_Hamiltonian::~Noise_Hamiltonian() = default;
