//
// Created by Rocky Su on 16/9/20.
//

#include "Hamiltonian.h"


/*********************************************Hamiltonian Classes*********************************************************/

void Hamiltonian::load_ext_waveform(std::string datapath) {

}

Hamiltonian::Hamiltonian(const Hamiltonian &h) {

}

Hamiltonian::~Hamiltonian() {
    arma::cx_mat().swap(h_mat);
    arma::cx_vec().swap(wave_form);
    arma::vec().swap(times_vec);
}

arma::cx_vec Hamiltonian::load_waveform() {
    arma::cx_vec wf = arma::cx_vec();
    if (external_waveform_path.length() >= 0) {
        wf.load(external_waveform_path,arma::hdf5_binary);
    }
    return wf;
}


arma::cx_vec MW_Hamiltonian::load_waveform() {
    arma::cx_vec wf =Hamiltonian::load_waveform();

    if(switching_signal.size() == num_of_steps) {
        const std::complex<double> j(0, 1);

        if(freq == 0.){
            for (int i = 0; i < wave_form.size(); ++i){
                wave_form[i] = step_size*amplitude*(get_amplitude(times_vec[i]) + get_amplitude(times_vec[i+1]))/2*std::exp(j*phase);
            }
        }
        else{
            for (int i = 0; i < wave_form.size(); ++i){
                wave_form[i] = amplitude*(get_amplitude(times_vec[i]) + get_amplitude(times_vec[i+1]))/2*std::exp(j*phase)/(j*freq*M_PI*2.)*(
                        std::exp(j*freq*2.*M_PI*(times_vec[i +1]))
                        -std::exp(j*freq*2.*M_PI*(times_vec[i])));
            }
        }
    }

    return wf;
//        pulse_data_conj = arma::conj(pulse_data);
}

double MW_Hamiltonian::get_amplitude(double time) const{
    if (modulation == undef){
        return 1;
    }
    if (modulation == gaussian){
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

MW_Hamiltonian::MW_Hamiltonian(const Hamiltonian &h, const MW_Hamiltonian &m): Hamiltonian(h) {

}
