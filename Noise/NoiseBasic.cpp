//
// Created by Rocky Su on 15/12/20.
//

#include "NoiseBasic.h"
#include <iostream>
#include <fstream>
#include <armadillo>

inline arma::vec _fft_freq(int n, double f){
    // n = number of points
    // f = frequency
    // return frequency components of the fourrier transform. (inspired from numpy implementation)
    // my_freq = [0, 1, ...,   n/2-1,     -n/2, ..., -1] *f/n   if n is even
    // my_freq = [0, 1, ..., (n-1)/2, -(n-1)/2, ..., -1] *f/n   if n is odd

    // determine number of elements in the left side of the sequence.
    int N = int(n/2) + n%2 -1;

    // Genreate left and right side of the sequence
    arma::vec my_freq_1 = arma::linspace<arma::vec>(0, N, N + 1);
    arma::vec my_freq_2 = arma::linspace<arma::vec>(- int(n/2) , -1, n-N-1);

    // Put the data together and make frequency absolute.
    arma::vec my_freq;
    my_freq = arma::join_cols<arma::mat>(my_freq_1,my_freq_2);
    my_freq /= n/f;

    return my_freq;
}

inline arma::vec _get_white_noise(double amplitude, int steps, double time_step){
    arma::vec white_noise(steps);

    // Make noise generator
    unsigned seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::default_random_engine generator(seed);

    double total_time = steps*time_step;
    std::normal_distribution<double> tmp(0, amplitude/std::sqrt(time_step));

    for (int i = 0; i < steps; ++i){
        white_noise[i] = tmp(generator);
    }

    return white_noise;
};

void NoiseBasic::load_config_from_path(std::string config_path) {
    config_file_path = config_path;
    nlohmann::json config_json;
    std::ifstream file(config_file_path);
    if (file) {
        file >> config_json;
        file.close();
    }
    config = config_json;
    tag = config["tag"];
    channels = config["channels"];
    start_idx = config["start_idx"];
    time_step = config["time_step"];
    alpha = config["alpha"];
    amp = config["amplitude"];
    noise_length = config["length"];
    export_dir = config["export_dir"];

    std::string copyConfigFileCommand = std::string("cp -v ").append(config_path).append(" ").append(export_dir).append("/").append(tag).append("_config.json");
    system(copyConfigFileCommand.c_str());
}

void NoiseBasic::generate_colored_noise() {
    // Due to the discretisation of the fourier transform, we want the noise to not we a frequency component of the simulation,
    // e.g. when we would take the fourrier components of a signal of 100 ns with steps of 1 ns
    // f1 = 1e7
    // f2 = 2e7
    // f... = 4.3e8
    // All these frequencies have in common that if you multiply them by 100ns, you will get an integer number, which means that the
    // when the 100ns are passed, exactly 2 pi has passed. This is unwanted behavoir.
    // This is resolved by making noise for a longer time (e.g. 10 times longer) with a random number of steps added on top

    //
    // Note that the method is inefficient.
    //

    // 1) make more steps
    int new_steps = 10*noise_length;
//	// 2) add random amount of steps
//  May cause deadlock!
//	unsigned seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
//	std::default_random_engine generator(seed);
//	std::normal_distribution<double> tmp(0, steps);
//	int add_on_steps = floor(std::abs(tmp(generator))/10) * 10;
//	new_steps += add_on_steps;

    // 1/f amplitude
    arma::vec freq_amp = _fft_freq(new_steps, 1/time_step);
    // Make sure the dc component is zero.
    if (alpha > 0) {
        freq_amp(0) = 1e300;
    } else {
        freq_amp(0) = 1e-300;
    }

    #pragma omp parallel for default(none) shared(new_steps,freq_amp,amp,time_step,tag,channels,start_idx,alpha,noise_length,export_dir)
    for (int i = 0; i < channels; ++i) {
        arma::vec one_f_noise_long;
        // Note % in armadillo, shur product!
        // note devide alpha by 2 since 1/f relation is related to the power spectrum and ~ V**2

        // gaussian white noise generated from gaussian distribution
        arma::vec white_noise(new_steps);
        white_noise = _get_white_noise(amp, new_steps, time_step);

        // FFT noise
        arma::cx_vec fft_white_noise = arma::fft(white_noise);
        // Note % in armadillo, shur product!
        // note devide alpha by 2 since 1/f relation is related to the power spectrum and ~ V**2
        fft_white_noise = fft_white_noise%arma::pow(arma::abs(1/freq_amp),alpha/2.);
        one_f_noise_long = arma::real(arma::ifft(fft_white_noise));

        arma::vec one_f_noise_cut(noise_length);
        int random_start_pos = rand() % (noise_length*8);

        one_f_noise_cut = one_f_noise_long.subvec(arma::span(random_start_pos,random_start_pos+noise_length-1));

        std::string fileName = std::string(export_dir).append(tag).append("#").append(std::to_string(i+start_idx)).append(".csv");
        one_f_noise_cut.save(fileName,arma::csv_ascii);
        printf("Noise: noise data write to:%s\n",fileName.c_str());
//        std::cout << "Noise: noise data write to:" << fileName << std::endl;
    }
}