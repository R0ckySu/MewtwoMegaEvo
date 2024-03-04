//
// Created by Rocky Su on 24/1/2024.
//

#ifndef MYPROJECT_MEWHAMILTONIAN_H
#define MYPROJECT_MEWHAMILTONIAN_H

#include <Wt/WContainerWidget.h>
#include <Wt/WTable.h>
#include "MewComponents.h"

class HamiltonianCell: public Wt::WContainerWidget {
public:
    int cell_index=0;
};

class MewHamiltonianTable: public Wt::WTable {
public:
    void reindexing_table_cells();
};

class StaticHamiltonianCell : public HamiltonianCell {
public:
    MewCellString *tag;
    MewCellBool *enable;
    MewCellDoubleNum *amplitude;
    MewCellString *h_pauli_mat;
    MewCellString *waveform_path;

    StaticHamiltonianCell(MewHamiltonianTable& parent_table);
    void loadInfoFromJson(Wt::Json::Object dict);
    Wt::Json::Object getJsonObj();
};

class MWHamiltonianCell : public HamiltonianCell {
public:
    MewCellString *tag;
    MewCellBool *enable;
    MewCellDoubleNum *amplitude;
    MewCellDoubleNum *rising_time;
    MewCellDoubleNum *falling_time;
    MewCellDoubleNum *freq;
    MewCellDoubleNum *phase;
    MewCellString *h_pauli_mat;
    MewCellString *waveform_path;

    MWHamiltonianCell(MewHamiltonianTable& parent_table);
    void loadInfoFromJson(Wt::Json::Object dict);
    Wt::Json::Object getJsonObj();
};

class AWGHamiltonianCell : public HamiltonianCell {
public:
    MewCellString *tag;
    MewCellBool *enable;
    MewCellDoubleNum *amplitude;
    MewCellDoubleNum *rising_time;
    MewCellDoubleNum *falling_time;
    MewCellString *h_pauli_mat;
    MewCellString *waveform_path;
    AWGHamiltonianCell(MewHamiltonianTable& parent_table);
    void loadInfoFromJson(Wt::Json::Object dict);
    Wt::Json::Object getJsonObj();
};

class NoiseHamiltonianCell : public HamiltonianCell {
public:
    MewCellString *tag;
    MewCellBool *enable;
    MewCellDoubleNum *amplitude;
    MewCellString *h_pauli_mat;
    MewCellDoubleNum *lag_time;
    MewCellString *waveform_path;
    NoiseHamiltonianCell(MewHamiltonianTable& parent_table);
    void loadInfoFromJson(Wt::Json::Object dict);
    Wt::Json::Object getJsonObj();
};

class MewHamiltonianConfig: public Wt::WContainerWidget {
public:
    MewHamiltonianTable* table;
    MewHamiltonianConfig();
    void load_from_file(std::string filePath);
    void dump_config();
};

#endif //MYPROJECT_MEWHAMILTONIAN_H
