//
// Created by Rocky Su on 20/9/20.
//

#include "Utils.h"
#include <regex>

//
// Created by Rocky Su on 2019/11/11.
//

namespace qmt{

    std::complex<double > ii(0,1);

    arma::cx_mat22 pauli_x() {
        arma::cx_mat pauli_x = arma::cx_mat(arma::zeros(2,2),arma::zeros(2,2));
        pauli_x(0,1) = 1;
        pauli_x(1,0) = 1;
        return pauli_x;
    }

    arma::cx_mat22 pauli_y() {
        arma::cx_mat pauli_y = arma::cx_mat(arma::zeros(2,2),arma::zeros(2,2));
        pauli_y(0,1) = -ii;
        pauli_y(1,0) = ii;
        return pauli_y;
    }

    arma::cx_mat22 pauli_z() {
        arma::cx_mat pauli_z = arma::cx_mat(arma::eye(2,2),arma::zeros(2,2));
        pauli_z(1,1) = -1;
        return pauli_z;
    }

    arma::vec heaviside(int x_0, int length) {
        arma::vec hs = arma::zeros(length);
        if(x_0 >= 0) {
            hs.subvec(arma::span(x_0,length-1)).fill(1);
        } else {
            hs.subvec(arma::span(0,-x_0)).fill(1);
        }
        return hs;
    }


    arma::cx_mat spinorDecoder(std::string spinorStr) {
        std::map<std::string,arma::cx_mat> pauliMap;
        pauliMap.insert(std::pair<std::string,arma::cx_mat>("I",arma::cx_mat(arma::eye(2,2),arma::zeros(2,2))));
        pauliMap.insert(std::pair<std::string,arma::cx_mat>("X",pauli_x()));
        pauliMap.insert(std::pair<std::string,arma::cx_mat>("Y",pauli_y()));
        pauliMap.insert(std::pair<std::string,arma::cx_mat>("Z",pauli_z()));

        arma::cx_mat spinor;

        bool imagFlag = false;
        if (spinorStr.substr(0,1)=="i"){
            imagFlag =true;
            spinorStr = spinorStr.substr(1,spinorStr.size());
        }

        int length = spinorStr.length();
        for (int i = length-1; i >=0; --i) {
            if (i==length-1) {
                spinor = pauliMap[spinorStr.substr(i,1)];
            } else {
                spinor = arma::kron(pauliMap[spinorStr.substr(i,1)],spinor);
            }
        }

        if (imagFlag) { spinor = spinor*arma::cx_double(0,1);}

        return spinor;
    }

    arma::cx_mat spinorExpressionDecoder(std::string expression){
        arma::cx_mat result;
        std::vector<std::string> operatorList;
        std::vector<std::string> spinorTermsList;
        std::string spinorTerm;

        int nextTermStartIdx =0;

        for (int i = 0; i < expression.size()-1; i++) {
            std::string a = expression.substr(i,1);
            if (a=="+" || a=="-") {

                if (i!= 0 && operatorList.size()==0) {
                    operatorList.push_back("+");
                }

                operatorList.push_back(a);

                if ((i-nextTermStartIdx) > 0){
                    spinorTermsList.push_back(expression.substr(nextTermStartIdx,i-nextTermStartIdx));
                }

                nextTermStartIdx = i+1;
            }
        }
        spinorTermsList.push_back(expression.substr(nextTermStartIdx,expression.size()-1));

        for (int j = 0; j < operatorList.size(); ++j) {
            int sign = 1;
            if(operatorList.at(j)=="+"){
                sign = 1;
            } else if(operatorList.at(j)=="-") {
                sign = -1;
            }

            if (j==0) {
                result = spinorDecoder(spinorTermsList.at(j))*sign;
            } else {
                result += spinorDecoder(spinorTermsList.at(j))*sign;
            }
//            std::cout<<operatorList.at(j)<<std::endl;
//            std::cout<<spinorTermsList.at(j)<<std::endl;
        }

        return result;
    }


    arma::cx_mat custom_matrix_exp(arma::cx_mat input_matrix) {
        // ok, but can be more efficient using the pade method.
        // uses now matrix scaling in combination with a taylor
        int accuracy = 10;

        const double norm_val = arma::norm(input_matrix, "inf");

        const double log2_val = (norm_val > 0.0) ? double(std::log2(norm_val)) : double(0);

        int exponent = int(0);  std::frexp(log2_val, &exponent);

        const int s = int( (std::max)(int(0), exponent + int(10)) );

        input_matrix = input_matrix/double(std::pow(double(2), double(s)));

        arma::mat tmp1(input_matrix.n_rows,input_matrix.n_rows);
        tmp1.eye();
        arma::mat tmp2(input_matrix.n_rows,input_matrix.n_rows);
        tmp2.zeros();
        arma::cx_mat tmp(tmp1,tmp2);
        arma::cx_mat output_matrix(tmp1,tmp2);

        double factorial_i = 1.0;

        for(int i = 1; i < accuracy; i++) {
            factorial_i = factorial_i * i;
            tmp *= input_matrix;
            output_matrix += tmp/factorial_i;
        }

        for(int i=0; i < s; ++i)  { output_matrix = output_matrix*output_matrix;}

        return output_matrix;
    }
}

std::vector<std::string> str_split(std::string s, char delimiter) {
    std::vector<std::string> splits = std::vector<std::string>();
    std::string split;
    std::istringstream ss(s);
    while (std::getline(ss, split, delimiter))
    {
        splits.push_back(split);
    }
    return splits;
}

arma::cx_mat load_matrix_from_config_str(std::string mat_string) {
    //TODO: Smart loading matrix from external file or Pauli symbol.
//    std::regex regPattern = std::regex(std::string("\\w([IXYZ])"));
//    arma::cx_mat mat = arma::cx_mat().fill(0);
//    if (std::regex_match(mat_string,regPattern)) {
//        mat = qmt::spinorDecoder(mat_string);
//    } else {
//        mat.load(mat_string,arma::csv_ascii);
//    }
    return qmt::spinorDecoder(mat_string);
};

std::vector<std::pair<std::string,std::string>> symbolic_sequence_decoder(std::string seq_expression){
//    int first_sq_bra = seq_expression.find_first_of('[');
//    int last_sq_ket = seq_expression.find_last_of(']');
//    std::string repeatable_sub_seq_str = seq_expression.substr(first_sq_bra+1,last_sq_ket-first_sq_bra-1);
//    std::cout << repeatable_sub_seq_str << std::endl;
    std::vector<std::string> symbolic_sequence_vec = str_split(seq_expression,'-');

    auto symbolic_seq_info_pair = std::vector<std::pair<std::string, std::string>>();
    for (const auto& gate_symbol : symbolic_sequence_vec) {
        std::string tag;
        std::string param_str;

        int left_par_idx = gate_symbol.find_first_of('(');
        int right_par_idx = gate_symbol.find_first_of(')');
        if ((left_par_idx != std::string::npos) && (right_par_idx != std::string::npos)) {
            tag = gate_symbol.substr(0,left_par_idx);
            param_str = gate_symbol.substr(left_par_idx+1,right_par_idx-left_par_idx-1);
        } else {
            tag = gate_symbol;
        }
        std::cout << tag << ":" << param_str << std::endl;
        symbolic_seq_info_pair.push_back(std::make_pair(tag,param_str));
    }

    return symbolic_seq_info_pair;
};

std::string get_time_stamp_str() {
    time_t rawtime;
    struct tm * timeinfo;
    char buffer[80];
    time (&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer,sizeof(buffer),"%Y%m%d%H%M%S",timeinfo);
    std::string time_str(buffer);
    return time_str;
};

std::string double_to_fixprecision_str(double num, int percision) {
    std::ostringstream double_str;
    double_str << std::setprecision(percision);
    double_str << std::scientific;
    double_str << num;
    return double_str.str();
}
