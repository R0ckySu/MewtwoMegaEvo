//
// Created by Rocky Su on 20/9/20.
//

#include "Utils.h"
#include <regex>

namespace qmt{

    std::complex<double > ii(0,1);

    arma::cx_mat22 pauli_x() {
        arma::cx_mat pauli_x = arma::cx_mat(arma::zeros(2,2),arma::zeros(2,2));
        pauli_x(0,1) = 0.5;
        pauli_x(1,0) = 0.5;
        return pauli_x;
    }

    arma::cx_mat22 pauli_y() {
        arma::cx_mat pauli_y = arma::cx_mat(arma::zeros(2,2),arma::zeros(2,2));
        pauli_y(0,1) = -ii*0.5;
        pauli_y(1,0) = ii*0.5;
        return pauli_y;
    }

    arma::cx_mat22 pauli_z() {
        arma::cx_mat pauli_z = arma::cx_mat(arma::eye(2,2)*0.5,arma::zeros(2,2));
        pauli_z(1,1) = -0.5;
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

std::vector<double> generate_random_num_list(int number_of_rand, int effective_digits) {
    int factor = pow(10,effective_digits);
    std::vector<double> rand_list = std::vector<double >(number_of_rand);
    srand((unsigned)time(NULL));
    for (int i1 = 0; i1 < number_of_rand; ++i1) {
        double randnum = rand()/double(RAND_MAX);
        rand_list.at(i1) = 0.8*floor(randnum*factor)/factor; // Downscale 80% to avoid out of range when have fixed shifted time
    }
    return rand_list;
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

void symbolic_matrix::load_from_symbol(std::string _symbol_name, std::string config_path) {
    symbol_name = std::move(_symbol_name);
    std::regex symbol_reg = std::regex("^[IXYZ]+");
    if (std::regex_match(symbol_name,symbol_reg)) {
        mat = qmt::spinorDecoder(symbol_name);
    } else {
        mat = arma::cx_mat();
        mat.load(std::string(config_path).append("/").append(symbol_name));
    }
}

symbolic_matrix::symbolic_matrix() {
    symbol_name="";
    mat=arma::cx_mat().fill(0);
}

std::vector<std::pair<std::string, std::string>> symbolic_sequence_str_parser (std::string sequence_str) {
    std::string seq_str = sequence_str;
    std::vector<std::pair<std::string, std::string>> elementary_gate_list = std::vector<std::pair<std::string, std::string>>();

    int left_first_bra_pos = seq_str.find_first_of('[');
    int right_last_ket_pos = seq_str.find_last_of(']');

    if ((left_first_bra_pos != seq_str.npos) && (right_last_ket_pos != seq_str.npos)) {
        std::string left_seq;
        if (left_first_bra_pos>0) {
            left_seq = seq_str.substr(0, left_first_bra_pos-1);
        }
//        std::cout << "Left:" << left_seq << std::endl;

        std::string right_string = seq_str.substr(right_last_ket_pos,seq_str.size()-right_last_ket_pos);
        int num_end_pos = right_string.size()-1;
//        std::cout << "Right raw string:" << right_string << std::endl;
        if (right_string.find_first_of('-') != right_string.npos) {
            num_end_pos = right_string.find_first_of('-')-1;
        }
//        std::cout << "Num str ends pos in right raw string:" << num_end_pos << std::endl;

        std::string right_seq;
        if (num_end_pos != right_string.size()-1) {
            right_seq = right_string.substr(num_end_pos+2,right_string.length()-(num_end_pos+2));
        }
        std::string num_of_repeat_str = right_string.substr(2,num_end_pos-1);
//        std::cout << "Right:" << right_seq << std::endl;

        int repeat_middle_num = atoi(num_of_repeat_str.c_str());

        seq_str = seq_str.substr(left_first_bra_pos+1,right_last_ket_pos-left_first_bra_pos-1);
//        std::cout << "Middle:" << seq_str << " Repeat for: "<< repeat_middle_num << std::endl;

        std::vector<std::string> splitted_gates_left = str_split(left_seq,'-');
        for (int i = 0; i < splitted_gates_left.size(); ++i) {
            elementary_gate_list.push_back(decompose_gate_string_to_tag_param_pair(splitted_gates_left.at(i)));
        }

        // Called recursively to unwrap the repeatable sub sequences
        std::vector<std::pair<std::string, std::string>> splitted_gates_mid = symbolic_sequence_str_parser(seq_str);
        for (int j = 0; j < repeat_middle_num; ++j) {
            for (int i = 0; i < splitted_gates_mid.size(); ++i) {
                elementary_gate_list.push_back(splitted_gates_mid.at(i));
            }
        }

        std::vector<std::string> splitted_gates_right = str_split(right_seq,'-');
        for (int i = 0; i < splitted_gates_right.size(); ++i) {
            elementary_gate_list.push_back(decompose_gate_string_to_tag_param_pair(splitted_gates_right.at(i)));
        }
    } else {
        std::vector<std::string> splitted_gates = str_split(seq_str,'-');
        for (int i = 0; i < splitted_gates.size(); ++i) {
            elementary_gate_list.push_back(decompose_gate_string_to_tag_param_pair(splitted_gates.at(i)));
        }
    }
    return elementary_gate_list;
}

std::pair<std::string, std::string> decompose_gate_string_to_tag_param_pair(const std::string& gate_str) {
    int first_para = gate_str.find_first_of('(');
    int last_para = gate_str.find_last_of(')');
    std::string tag;
    std::string param;
    if (first_para!= std::string::npos && last_para!=std::string::npos) {
        tag = gate_str.substr(0,first_para);
        param = gate_str.substr(first_para+1,last_para-first_para-1);
    } else {
        tag = gate_str;
    }
    return std::make_pair(tag, param);
}

std::string find_and_replace_string(const std::string& str_to_find, const std::string& str_to_rep, std::string original_str){
    std::string replaced_str = original_str;
    std::string::size_type pos = 0;
    std::string::size_type srclen = str_to_find.size();
    std::string::size_type dstlen = str_to_rep.size();
    while ((pos = replaced_str.find(str_to_find, pos)) != std::string::npos) {
        replaced_str.replace(pos, srclen, str_to_rep);
        pos += dstlen;
    }
    return replaced_str;
};