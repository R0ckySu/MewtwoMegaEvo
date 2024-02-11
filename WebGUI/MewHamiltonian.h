//
// Created by Rocky Su on 24/1/2024.
//

#ifndef MYPROJECT_MEWHAMILTONIAN_H
#define MYPROJECT_MEWHAMILTONIAN_H

#include <Wt/WContainerWidget.h>
#include <Wt/WPushButton.h>
#include <Wt/WTable.h>
#include <Wt/WVBoxLayout.h>
#include <Wt/WHBoxLayout.h>
#include <Wt/WAnimation.h>
#include <Wt/Json/Parser.h>
#include <Wt/Json/Value.h>
#include <Wt/Json/Serializer.h>
#include "MewComponents.h"
#include <fstream>


class HamiltonianCell: public Wt::WContainerWidget {
public:
    int cell_index=0;
};

class MewHamiltonianTable: public Wt::WTable {
public:
    void reindexing_table_cells() {
        for (int i=0; i<rowCount(); i++) {
            if(dynamic_cast<HamiltonianCell*>(elementAt(i, 0)->widget(0)) != nullptr) {
                HamiltonianCell* cell = (HamiltonianCell*)elementAt(i, 0)->widget(0);
                cell->cell_index = i;
            }
        }
    }
};


class StaticHamiltonianCell : public HamiltonianCell {
public:
    MewCellString *tag;
    MewCellBool *enable;
    MewCellDoubleNum *amplitude;
    MewCellString *h_pauli_mat;
    MewCellString *waveform_path;

    StaticHamiltonianCell(MewHamiltonianTable& parent_table) : HamiltonianCell() {
//        setAttributeValue("style", "background-color: #d9ed92;");
        auto panel = addWidget(std::make_unique<Wt::WPanel>());
        panel->setTitle(Wt::WString("Static Hamiltonian"));
        panel->setCollapsed(true);
        auto panel_Container = panel->setCentralWidget(std::make_unique<Wt::WContainerWidget>());
        panel->titleBarWidget()->setAttributeValue("style", "background-color: #d9ed92;");

        auto vLayout =panel_Container->setLayout(std::make_unique<Wt::WVBoxLayout>());
        tag = vLayout->addWidget(std::make_unique<MewCellString>("tag", ""));
        enable = vLayout->addWidget(std::make_unique<MewCellBool>("enable", false));
        amplitude = vLayout->addWidget(std::make_unique<MewCellDoubleNum>("amplitude", 0));
        h_pauli_mat = vLayout->addWidget(std::make_unique<MewCellString>("h_pauli_mat", ""));
        waveform_path = vLayout->addWidget(std::make_unique<MewCellString>("waveform_path", ""));

        auto deleteButton = vLayout->addWidget(std::make_unique<Wt::WPushButton>("Delete"));
        deleteButton->clicked().connect([=, &parent_table] {
            std::cout << "Deleting at " << cell_index << std::endl;
            parent_table.removeRow(cell_index);
            parent_table.reindexing_table_cells();
        });
    }

    void loadInfoFromJson(Wt::Json::Object dict) {
        tag->set_data(dict.get(std::string(tag->get_data().first)));
        enable->set_data(dict.get(std::string(enable->get_data().first)));
        amplitude->set_data(dict.get(std::string(amplitude->get_data().first)));
        h_pauli_mat->set_data(dict.get(std::string(h_pauli_mat->get_data().first)));
        waveform_path->set_data(dict.get(std::string(waveform_path->get_data().first)));
    }

    Wt::Json::Object getJsonObj() {
        Wt::Json::Object h = Wt::Json::Object();
        h.insert(tag->get_data());
        h.insert(std::make_pair("type","static"));
        h.insert(enable->get_data());
        h.insert(amplitude->get_data());
        h.insert(h_pauli_mat->get_data());
        h.insert(waveform_path->get_data());
        return h;
    }
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

    MWHamiltonianCell(MewHamiltonianTable& parent_table) : HamiltonianCell() {

        auto panel = addWidget(std::make_unique<Wt::WPanel>());
        panel->setTitle(Wt::WString("Microwave Hamiltonian"));
        panel->setCollapsed(true);
        auto panel_Container = panel->setCentralWidget(std::make_unique<Wt::WContainerWidget>());
        panel->titleBarWidget()->setAttributeValue("style", "background-color: #b5e48c;");

        auto vLayout = panel_Container->setLayout(std::make_unique<Wt::WVBoxLayout>());
        tag = vLayout->addWidget(std::make_unique<MewCellString>("tag",""));
        enable = vLayout->addWidget(std::make_unique<MewCellBool>("enable", false));
        amplitude = vLayout->addWidget(std::make_unique<MewCellDoubleNum>("amplitude",0));
        rising_time = vLayout->addWidget(std::make_unique<MewCellDoubleNum>("rising_time",0));
        falling_time = vLayout->addWidget(std::make_unique<MewCellDoubleNum>("falling_time",0));
        freq = vLayout->addWidget(std::make_unique<MewCellDoubleNum>("freq",0));
        phase = vLayout->addWidget(std::make_unique<MewCellDoubleNum>("phase",0));
        h_pauli_mat = vLayout->addWidget(std::make_unique<MewCellString>("h_pauli_mat",""));
        waveform_path =vLayout->addWidget(std::make_unique<MewCellString>("waveform_path",""));
        auto deleteButton = vLayout->addWidget(std::make_unique<Wt::WPushButton>("Delete"));
        deleteButton->clicked().connect([=, &parent_table] {
            std::cout << "Deleting at " << cell_index << std::endl;
            parent_table.removeRow(cell_index);
            parent_table.reindexing_table_cells();
        });
    }

    void loadInfoFromJson(Wt::Json::Object dict) {
        tag->set_data(dict.get(std::string(tag->get_data().first)));
        enable->set_data(dict.get(std::string(enable->get_data().first)));
        amplitude->set_data(dict.get(std::string(amplitude->get_data().first)));
        rising_time->set_data(dict.get(std::string(rising_time->get_data().first)));
        falling_time->set_data(dict.get(std::string(falling_time->get_data().first)));
        freq->set_data(dict.get(std::string(freq->get_data().first)));
        phase->set_data(dict.get(std::string(phase->get_data().first)));
        h_pauli_mat->set_data(dict.get(std::string(h_pauli_mat->get_data().first)));
        waveform_path->set_data(dict.get(std::string(waveform_path->get_data().first)));
    }

    Wt::Json::Object getJsonObj() {
        Wt::Json::Object h = Wt::Json::Object();
        h.insert(tag->get_data());
        h.insert(std::make_pair("type","mw"));
        h.insert(enable->get_data());
        h.insert(amplitude->get_data());
        h.insert(rising_time->get_data());
        h.insert(falling_time->get_data());
        h.insert(freq->get_data());
        h.insert(phase->get_data());
        h.insert(h_pauli_mat->get_data());
        h.insert(waveform_path->get_data());
        return h;
    }
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

    AWGHamiltonianCell(MewHamiltonianTable& parent_table) : HamiltonianCell() {
//        setAttributeValue("style", "background-color: #99d98c;");
        auto panel = addWidget(std::make_unique<Wt::WPanel>());
        panel->setTitle(Wt::WString("Microwave Hamiltonian"));
        panel->setCollapsed(true);
        auto panel_Container = panel->setCentralWidget(std::make_unique<Wt::WContainerWidget>());
        panel->titleBarWidget()->setAttributeValue("style", "background-color: #99d98c;");

        auto vLayout = panel_Container->setLayout(std::make_unique<Wt::WVBoxLayout>());
        tag = vLayout->addWidget(std::make_unique<MewCellString>("tag",""));
        enable = vLayout->addWidget(std::make_unique<MewCellBool>("enable", false));
        amplitude = vLayout->addWidget(std::make_unique<MewCellDoubleNum>("amplitude",0));
        rising_time = vLayout->addWidget(std::make_unique<MewCellDoubleNum>("rising_time",0));
        falling_time = vLayout->addWidget(std::make_unique<MewCellDoubleNum>("falling_time",0));
        h_pauli_mat = vLayout->addWidget(std::make_unique<MewCellString>("h_pauli_mat",""));
        waveform_path =vLayout->addWidget(std::make_unique<MewCellString>("waveform_path",""));
        auto deleteButton = vLayout->addWidget(std::make_unique<Wt::WPushButton>("Delete"));
        deleteButton->clicked().connect([=, &parent_table] {
            std::cout << "Deleting at " << cell_index << std::endl;
            parent_table.removeRow(cell_index);
            parent_table.reindexing_table_cells();
        });
    }

    void loadInfoFromJson(Wt::Json::Object dict) {
        tag->set_data(dict.get(std::string(tag->get_data().first)));
        enable->set_data(dict.get(std::string(enable->get_data().first)));
        amplitude->set_data(dict.get(std::string(amplitude->get_data().first)));
        rising_time->set_data(dict.get(std::string(rising_time->get_data().first)));
        falling_time->set_data(dict.get(std::string(falling_time->get_data().first)));
        h_pauli_mat->set_data(dict.get(std::string(h_pauli_mat->get_data().first)));
        waveform_path->set_data(dict.get(std::string(waveform_path->get_data().first)));
    }

    Wt::Json::Object getJsonObj() {
        Wt::Json::Object h = Wt::Json::Object();
        h.insert(tag->get_data());
        h.insert(std::make_pair("type","awg"));
        h.insert(enable->get_data());
        h.insert(amplitude->get_data());
        h.insert(rising_time->get_data());
        h.insert(falling_time->get_data());
        h.insert(h_pauli_mat->get_data());
        h.insert(waveform_path->get_data());
        return h;
    }
};

class NoiseHamiltonianCell : public HamiltonianCell {
public:
    MewCellString *tag;
    MewCellBool *enable;
    MewCellDoubleNum *amplitude;
    MewCellString *h_pauli_mat;
    MewCellDoubleNum *lag_time;
    MewCellString *waveform_path;

    NoiseHamiltonianCell(MewHamiltonianTable& parent_table) : HamiltonianCell() {
//        setAttributeValue("style", "background-color: #76c893;");

        auto panel = addWidget(std::make_unique<Wt::WPanel>());
        panel->setTitle(Wt::WString("Noise Hamiltonian"));
        panel->setCollapsed(true);
        auto panel_Container = panel->setCentralWidget(std::make_unique<Wt::WContainerWidget>());
        panel->titleBarWidget()->setAttributeValue("style", "background-color: #76c893;");

        auto vLayout = panel_Container->setLayout(std::make_unique<Wt::WVBoxLayout>());
        tag = vLayout->addWidget(std::make_unique<MewCellString>("tag",""));
        enable = vLayout->addWidget(std::make_unique<MewCellBool>("enable", false));
        amplitude = vLayout->addWidget(std::make_unique<MewCellDoubleNum>("amplitude",0));
        h_pauli_mat = vLayout->addWidget(std::make_unique<MewCellString>("h_pauli_mat",""));
        lag_time = vLayout->addWidget(std::make_unique<MewCellDoubleNum>("lag_time",0));
        waveform_path = vLayout->addWidget(std::make_unique<MewCellString>("waveform_path",""));
        auto deleteButton = vLayout->addWidget(std::make_unique<Wt::WPushButton>("Delete"));
        deleteButton->clicked().connect([=, &parent_table] {
            std::cout << "Deleting at " << cell_index << std::endl;
            parent_table.removeRow(cell_index);
            parent_table.reindexing_table_cells();
        });
    }

    void loadInfoFromJson(Wt::Json::Object dict) {
        tag->set_data(dict.get(std::string(tag->get_data().first)));
        enable->set_data(dict.get(std::string(enable->get_data().first)));
        amplitude->set_data(dict.get(std::string(amplitude->get_data().first)));
        h_pauli_mat->set_data(dict.get(std::string(h_pauli_mat->get_data().first)));
        lag_time->set_data(dict.get(std::string(lag_time->get_data().first)));
        waveform_path->set_data(dict.get(std::string(waveform_path->get_data().first)));
    }

    Wt::Json::Object getJsonObj() {
        Wt::Json::Object h = Wt::Json::Object();
        h.insert(tag->get_data());
        h.insert(std::make_pair("type","noise"));
        h.insert(enable->get_data());
        h.insert(amplitude->get_data());
        h.insert(h_pauli_mat->get_data());
        h.insert(lag_time->get_data());
        h.insert(waveform_path->get_data());
        return h;
    }
};


class MewHamiltonianConfig: public Wt::WContainerWidget {
public:
    MewHamiltonianTable* table;
    MewHamiltonianConfig() : Wt::WContainerWidget() {
        setAttributeValue("style", "background-color: #c0dfe7;");
//        auto saveButton = addWidget(std::make_unique<Wt::WPushButton>("Save File"));
//        saveButton->clicked().connect([=] {
//            dump_config();
//        });

        auto buttonContainer= addWidget(std::make_unique<Wt::WContainerWidget>());
        auto hButtonLayout = buttonContainer->setLayout(std::make_unique<Wt::WHBoxLayout>());
        auto addButtonStatic= hButtonLayout->addWidget(std::make_unique<Wt::WPushButton>("+ Static"));
        addButtonStatic->setAttributeValue("style", "background-color: #d9ed92;");
        auto addButtonMW = hButtonLayout->addWidget(std::make_unique<Wt::WPushButton>("+ MW"));
        addButtonMW->setAttributeValue("style", "background-color: #b5e48c;");
        auto addButtonAWG = hButtonLayout->addWidget(std::make_unique<Wt::WPushButton>("+ AWG"));
        addButtonAWG->setAttributeValue("style", "background-color: #99d98c;");
        auto addButtonNoise = hButtonLayout->addWidget(std::make_unique<Wt::WPushButton>("+ Noise"));
        addButtonNoise->setAttributeValue("style", "background-color: #76c893;");

        table = addWidget(std::make_unique<MewHamiltonianTable>());
        table->setAttributeValue("style", "border-collapse: separate; border-spacing: 5px 5px;");

        addButtonStatic->clicked().connect([=] {
            int newRow = table->rowCount();
            table->insertRow(newRow);
            table->elementAt(newRow, 0)->addWidget(std::make_unique<StaticHamiltonianCell>(*table));
            table->reindexing_table_cells();
        });

        addButtonMW->clicked().connect([=] {
            int newRow = table->rowCount();
            table->insertRow(newRow);
            table->elementAt(newRow, 0)->addWidget(std::make_unique<MWHamiltonianCell>(*table));
            table->reindexing_table_cells();
        });

        addButtonAWG->clicked().connect([=] {
            int newRow = table->rowCount();
            table->insertRow(newRow);
            table->elementAt(newRow, 0)->addWidget(std::make_unique<AWGHamiltonianCell>(*table));
            table->reindexing_table_cells();
        });

        addButtonNoise->clicked().connect([=] {
            int newRow = table->rowCount();
            table->insertRow(newRow);
            table->elementAt(newRow, 0)->addWidget(std::make_unique<NoiseHamiltonianCell>(*table));
            table->reindexing_table_cells();
        });
    }

    void load_from_file(std::string filePath) {
        std::ifstream file(filePath);
        std::stringstream buffer;

        if (file) {
            buffer << file.rdbuf();
        } else {
            throw std::runtime_error("Unable to open file: " + filePath);
        }
        std::string jsonString = buffer.str();

        Wt::Json::Object jsonObject;
        Wt::Json::ParseError error;
        if (!Wt::Json::parse(jsonString, jsonObject, error)) {
            throw std::runtime_error("JSON parsing error: " + std::string(error.what()));
        }

        Wt::Json::Array H_list_json = jsonObject["hamiltonian_prototype_defs"];
        table->clear();
        int row_cnt = 0;
        for (const auto& h_obj : H_list_json) {
            if (h_obj.type() == Wt::Json::Type::Object) {
                auto h_obj_s = Wt::Json::Object(h_obj);
                if (h_obj_s.get("type").toString() == "static") {
                    std::cout << "Static H Detected!" << std::endl;
                    table->insertRow(row_cnt);
                    StaticHamiltonianCell *newStaticCell = table->elementAt(row_cnt, 0)->addWidget(
                            std::make_unique<StaticHamiltonianCell>(*table));
                    newStaticCell->loadInfoFromJson(h_obj_s);
                } else if (h_obj_s.get("type").toString() == "mw") {
                    std::cout << "MW H Detected!" << std::endl;
                    table->insertRow(row_cnt);
                    MWHamiltonianCell *newMWCell = table->elementAt(row_cnt, 0)->addWidget(
                            std::make_unique<MWHamiltonianCell>(*table));
                    newMWCell->loadInfoFromJson(h_obj_s);
                } else if (h_obj_s.get("type").toString() == "awg") {
                    std::cout << "AWG H Detected!" << std::endl;
                    table->insertRow(row_cnt);
                    AWGHamiltonianCell *newAWGCell = table->elementAt(row_cnt, 0)->addWidget(
                            std::make_unique<AWGHamiltonianCell>(*table));
                    newAWGCell->loadInfoFromJson(h_obj_s);
                } else if (h_obj_s.get("type").toString() == "noise") {
                    std::cout << "Noise H Detected!" << std::endl;
                    table->insertRow(row_cnt);
                    NoiseHamiltonianCell *newNoiseCell = table->elementAt(row_cnt, 0)->addWidget(
                            std::make_unique<NoiseHamiltonianCell>(*table));
                    newNoiseCell->loadInfoFromJson(h_obj_s);
                }
                row_cnt ++;
            }
        }
        table->reindexing_table_cells();
    }


    void dump_config() {
        Wt::Json::Object hamiltonian_config;

        auto hamiltonian_List = Wt::Json::Array();
        for (int i=0; i<table->rowCount(); i++) {
            if(dynamic_cast<StaticHamiltonianCell*>(table->elementAt(i, 0)->widget(0)) != nullptr) {
                hamiltonian_List.push_back(((StaticHamiltonianCell*)(table->elementAt(i, 0)->widget(0)))->getJsonObj());
            } else if(dynamic_cast<MWHamiltonianCell*>(table->elementAt(i, 0)->widget(0)) != nullptr) {
                hamiltonian_List.push_back(((MWHamiltonianCell*)(table->elementAt(i, 0)->widget(0)))->getJsonObj());
            } else if(dynamic_cast<AWGHamiltonianCell*>(table->elementAt(i, 0)->widget(0)) != nullptr) {
                hamiltonian_List.push_back(((AWGHamiltonianCell*)(table->elementAt(i, 0)->widget(0)))->getJsonObj());
            } else if(dynamic_cast<NoiseHamiltonianCell*>(table->elementAt(i, 0)->widget(0)) != nullptr) {
                hamiltonian_List.push_back(((NoiseHamiltonianCell*)(table->elementAt(i, 0)->widget(0)))->getJsonObj());
            }
        }
        hamiltonian_config["hamiltonian_prototype_defs"] = hamiltonian_List;

        std::string jsonString = Wt::Json::serialize(hamiltonian_config, true);
        std::ofstream file("hamiltonian_config.json");
        if (file.is_open()) {
            file << jsonString;
            file.close();
        } else {
            // Error handling
            std::cerr << "Unable to open file for writing." << std::endl;
        }
    }

};

#endif //MYPROJECT_MEWHAMILTONIAN_H
