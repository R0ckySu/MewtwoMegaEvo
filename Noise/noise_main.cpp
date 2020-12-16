//
// Created by Rocky Su on 15/12/20.
//
#include <iostream>
#include <getopt.h>
#include <unistd.h>
#include "NoiseBasic.h"

const char* short_options = "c:";
const struct option long_options[] = {
        {"config_file", 1, NULL,'c'},
        {nullptr,         0, nullptr, 0}
};

int main(int argc, char *argv[]) {
    NoiseBasic *noise = new NoiseBasic();

    char *current_path = getcwd(NULL,0);
    std::string default_config_file = std::string(current_path).append("/noise_config.json");

    int c;
    while((c = getopt_long(argc, argv, short_options, long_options, nullptr)) != -1) {
        printf("[+]Get sim task startup options : %d \n", c);
        switch (c) {
            case 'c':
                std::cout << "Config File Path:" << optarg << std::endl;
                default_config_file  = optarg;
                break;
            default:
                break;
        }
    }
    noise->load_config_from_path(default_config_file);
    noise->generate_colored_noise();
}