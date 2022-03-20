//
// Created by Rocky Su on 20/3/2022.
//

#ifndef MEWTWOMEGAEVO_EXTSIGCACHE_H
#define MEWTWOMEGAEVO_EXTSIGCACHE_H

#include <iostream>
#include <armadillo>

class ExtSigCache {
public:
    ExtSigCache();
    ~ExtSigCache();
    std::map<std::string, arma::cx_vec> cached_ext_signal_map;
//    void cache(std::string ext_file_name, arma::vec ext_sig);
    arma::cx_vec load_from_cache(std::string ext_file_name);
    void clean_cache();
};


#endif //MEWTWOMEGAEVO_EXTSIGCACHE_H
