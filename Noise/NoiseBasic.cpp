//
// Created by Rocky Su on 15/12/20.
//

#include "NoiseBasic.h"
#include <iostream>
#include <fstream>
//#include <exprtk/exprtk.hpp>
#include <muparserx/mpParser.h>

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
    amp = config["amplitude"];
    noise_length = config["length"];
    export_dir = config["export_dir"];
    noise_gen_mode = config["mode"];

    if (noise_gen_mode == NOISE_MODE_ARB) {
        arb_noise_expr = config["noise_expr"];
    } else if (noise_gen_mode == NOISE_MODE_COLORED) {
        alpha = config["alpha"];
    }

    std::string copyConfigFileCommand = std::string("cp -v ").append(config_path).append(" ").append(export_dir).append("/").append(tag).append("_config.json");
    system(copyConfigFileCommand.c_str());
}

void NoiseBasic::generate_arb_noise_amp_func() {

    mup::Value freq = 0.000001;
    mup::ParserX p;
    p.DefineVar("f", mup::Variable(&freq));
    p.SetExpr(arb_noise_expr);

    int new_steps = 10*noise_length;
    arma::vec freq_vec = _fft_freq(new_steps, 1/time_step);
    // Make sure the dc component is zero.
    double zero_freq_val = 0;
    try
    {   zero_freq_val = p.Eval().GetFloat();
        std::cout << p.Eval() << std::endl;
    }
    catch (mup::ParserError &e)
    {
        std::cout << e.GetMsg() << std::endl;
    }

    if (zero_freq_val > 1) {
        freq_vec(0) = 1e300;
    } else {
        freq_vec(0) = 1e-300;
    }

    arma::cx_vec arb_noise_func = arma::cx_vec(freq_vec.n_elem);
    for (int i = 0; i < freq_vec.n_elem; ++i) {
        freq = freq_vec.at(i);
        arb_noise_func.at(i) = p.Eval().GetComplex();
//        std::cout << arb_noise_func.at(i) << std::endl;
    }

    gen_reamped_white_noise_with_spec_func(arb_noise_func);
}

void NoiseBasic::generate_colored_noise_amp_func() {
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

    arma::vec amp_func = arma::pow(arma::abs(1/freq_amp),alpha/2.);
    gen_reamped_white_noise_with_spec_func(arma::cx_vec(amp_func,arma::zeros(amp_func.n_elem,1)));
}

void NoiseBasic::generate_noise() {
    if (noise_gen_mode == NOISE_MODE_COLORED) {
        generate_colored_noise_amp_func();
    } else if (noise_gen_mode == NOISE_MODE_ARB) {
        generate_arb_noise_amp_func();
    }
}

void NoiseBasic::gen_reamped_white_noise_with_spec_func(arma::cx_vec amp_func) {
    int new_steps = amp_func.size();
    std::string fileName = std::string(export_dir).append(tag);

    std::string spec_amp_file_name = std::string(fileName).append("_spectrum_amp.csv");
    amp_func.save(spec_amp_file_name,arma::csv_ascii);
    printf("Noise: noise spectrum amp write to:%s\n",spec_amp_file_name.c_str());

    #pragma omp parallel for default(none) shared(fileName,new_steps,amp_func,amp,time_step,tag,channels,start_idx,alpha,noise_length,export_dir)
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
        fft_white_noise = fft_white_noise%amp_func;
        one_f_noise_long = arma::real(arma::ifft(fft_white_noise));

        arma::vec one_f_noise_cut(noise_length);
        int random_start_pos = rand() % (noise_length*8);

        one_f_noise_cut = one_f_noise_long.subvec(arma::span(random_start_pos,random_start_pos+noise_length-1));

        std::string ith_noise_fileName = std::string(fileName).append("#").append(std::to_string(i+start_idx)).append(".csv");
        one_f_noise_cut.save(ith_noise_fileName,arma::csv_ascii);
        printf("Noise: noise data write to:%s\n",ith_noise_fileName.c_str());
//        std::cout << "Noise: noise data write to:" << fileName << std::endl;
    }
}
