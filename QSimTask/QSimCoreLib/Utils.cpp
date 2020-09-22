//
// Created by Rocky Su on 20/9/20.
//

#include "Utils.h"

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

}

std::vector<std::string> str_split(std::string s, char delimiter) {
    std::vector<std::string> splits;
    std::string split;
    std::istringstream ss(s);
    while (std::getline(ss, split, delimiter))
    {
        splits.push_back(split);
    }
    return splits;
}