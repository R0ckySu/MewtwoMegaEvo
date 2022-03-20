//
// Created by Rocky Su on 20/3/2022.
//

#include "ExtSigCache.h"

ExtSigCache::ExtSigCache(){
    cached_ext_signal_map = std::map<std::string, arma::cx_vec>();
};

ExtSigCache::~ExtSigCache() {
    clean_cache();
}

arma::cx_vec ExtSigCache::load_from_cache(std::string ext_file_name) {
    if (cached_ext_signal_map.find(ext_file_name) == cached_ext_signal_map.end()) {
        arma::cx_vec ext_sig = arma::cx_vec();
        #pragma omp critical
        {
            ext_sig.load(ext_file_name,arma::csv_ascii);
        };
        cached_ext_signal_map[ext_file_name] = ext_sig;
        return  cached_ext_signal_map[ext_file_name];
    } else {
        return cached_ext_signal_map[ext_file_name];
    }
}

void ExtSigCache::clean_cache() {
    for(auto &item : cached_ext_signal_map) {
        arma::cx_vec().swap(cached_ext_signal_map[item.first]);
    }
}
