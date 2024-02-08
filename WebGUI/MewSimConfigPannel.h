//
// Created by Rocky Su on 3/6/2023.
//

#ifndef MYPROJECT_MEWSIMCONFIGPANNEL_H
#define MYPROJECT_MEWSIMCONFIGPANNEL_H
//#include <iostream>
#include "MewComponents.h"
#include <Wt/WContainerWidget.h>
#include <Wt/WPushButton.h>
#include <Wt/WTable.h>
#include <Wt/WVBoxLayout.h>
#include <Wt/WHBoxLayout.h>
#include <Wt/Json/Object.h>
#include <Wt/Json/Array.h>
#include <Wt/Json/Parser.h>
//#include <Wt/Json/Value.h>
#include <Wt/Json/Serializer.h>
#include <fstream>
#include <string>

Wt::Json::Object reverseEntries(const Wt::Json::Object& original) {
    // Extract key-value pairs into a vector
    std::vector<std::pair<std::string, Wt::Json::Value>> entries;
    for (const auto& kv : original) {
        entries.push_back(kv);
    }

    // Reverse the vector of entries
    std::reverse(entries.begin(), entries.end());

    // Create a new Json::Object and insert the reversed entries
    Wt::Json::Object reversed;
    for (const auto& kv : entries) {
        reversed[kv.first] = kv.second;
    }

    return reversed;
}

class MewParamConfigCell : public Wt::WContainerWidget {
public:
    int cell_index;
};

class MewParamTable: public Wt::WTable {
public:
    void reindexing_table_cells() {
        for (int i=0; i<rowCount(); i++) {
            if(dynamic_cast<MewParamConfigCell*>(elementAt(i, 0)->widget(0)) != nullptr) {
                MewParamConfigCell* cell = (MewParamConfigCell*)elementAt(i, 0)->widget(0);
                cell->cell_index = i;
            }
        }
    }
};

class MewParamConfigCellStr : public MewParamConfigCell {
public:
    MewCellString *class_type;
    MewCellString *tag;
    MewCellString *property;
    MewCellString *string_file;

    MewParamConfigCellStr(MewParamTable& parent_table) : MewParamConfigCell() {
        class_type = addWidget(std::make_unique<MewCellString>("class", ""));
        tag = addWidget(std::make_unique<MewCellString>("tag", ""));
        property = addWidget(std::make_unique<MewCellString>("property", ""));
        string_file = addWidget(std::make_unique<MewCellString>("string_file", ""));

        auto deleteButton = addWidget(std::make_unique<Wt::WPushButton>("Delete"));
        deleteButton->clicked().connect([=, &parent_table] {
            parent_table.removeRow(cell_index);
            parent_table.reindexing_table_cells();
        });
    }

    void load_data(Wt::Json::Object data) {
        class_type->set_data(data["class"]);
        tag->set_data(data["tag"]);
        property->set_data(data["property"]);
        string_file->set_data(data["string_file"]);
    }

    Wt::Json::Object get_obj() {
        Wt::Json::Object obj;
        obj["class"] = Wt::Json::Value(class_type->get_data().second);
        obj["tag"] = Wt::Json::Value(tag->get_data().second);
        obj["property"] = Wt::Json::Value(property->get_data().second);
        obj["string_file"] = Wt::Json::Value(string_file->get_data().second);
        return obj;
    }
};

class MewParamConfigCellNum : public MewParamConfigCell {
public:
    MewCellString *class_type;
    MewCellString *tag;
    MewCellString *property;
    MewCellString *val_file;

    MewParamConfigCellNum(MewParamTable& parent_table) : MewParamConfigCell() {
        class_type = addWidget(std::make_unique<MewCellString>("class", ""));
        tag = addWidget(std::make_unique<MewCellString>("tag", ""));
        property = addWidget(std::make_unique<MewCellString>("property", ""));
        val_file = addWidget(std::make_unique<MewCellString>("val_file", ""));
        auto deleteButton = addWidget(std::make_unique<Wt::WPushButton>("Delete"));
        deleteButton->clicked().connect([=, &parent_table] {
            parent_table.removeRow(cell_index);
            parent_table.reindexing_table_cells();
        });
    }

    void load_data(Wt::Json::Object data) {
        class_type->set_data(data["class"]);
        tag->set_data(data["tag"]);
        property->set_data(data["property"]);
        val_file->set_data(data["string_file"]);
    }

    Wt::Json::Object get_obj() {
        Wt::Json::Object obj;
        obj["class"] = Wt::Json::Value(class_type->get_data().second);
        obj["tag"] = Wt::Json::Value(tag->get_data().second);
        obj["property"] = Wt::Json::Value(property->get_data().second);
        obj["string_file"] = Wt::Json::Value(val_file->get_data().second);
        return obj;
    }
};

class MewSimConfigPannel : public Wt::WContainerWidget {
public:
    MewCellString *task_name;
    MewCellIntNum *log_level;
    MewOptions *job_slicing_strategy;
    MewCellBool *record_propagator;
    MewCellBool *record_all_meas;
    MewCellBool *record_density_mat;
    MewCellBool *enable_param_parallel_mode;
    MewCellIntNum *system_dim;
    MewStrList *observables;
    MewStrList *init_states;
    MewCellIntNum *repeat;
    MewCellDoubleNum *step_size;
    MewCellString *sequence;

    Wt::WPushButton *addNumParam;
    Wt::WPushButton *addStrParam;
    MewParamTable *paramTable;

    MewSimConfigPannel(Wt::WContainerWidget* parent = nullptr) : Wt::WContainerWidget() {
        setAttributeValue("style", "background-color: #b5e48c;");
        auto saveButton = addWidget(std::make_unique<Wt::WPushButton>("Save File"));
        saveButton->clicked().connect([=] {
            dump_config();
        });
        auto table1 = addWidget(std::make_unique<Wt::WTable>());
        table1->setAttributeValue("style", "border-collapse: separate; border-spacing: 5px 5px;");

        task_name = table1->elementAt(0, 0)->addWidget(std::make_unique<MewCellString>("task_name",""));
        log_level = table1->elementAt(1, 0)->addWidget(std::make_unique<MewCellIntNum>("log_level", 4));

        std::vector<std::string> job_slice_opt = std::vector<std::string>();
        job_slice_opt.push_back(std::string("linspace"));
        job_slice_opt.push_back(std::string("logspace"));
        job_slice_opt.push_back(std::string("inv_logspace"));
        job_slicing_strategy = table1->elementAt(2, 0)->addWidget(std::make_unique<MewOptions>("job_slicing_strategy",job_slice_opt));
        record_propagator = table1->elementAt(3, 0)->addWidget(std::make_unique<MewCellBool>("record_propagator", false));
        record_all_meas = table1->elementAt(4, 0)->addWidget(std::make_unique<MewCellBool>("record_all_meas", false));
        record_density_mat = table1->elementAt(5, 0)->addWidget(std::make_unique<MewCellBool>("record_density_mat", false));
        enable_param_parallel_mode = table1->elementAt(6, 0)->addWidget(std::make_unique<MewCellBool>("enable_param_parallel_mode", false));

        system_dim = table1->elementAt(7, 0)->addWidget(std::make_unique<MewCellIntNum>("system_dim", 2));
        observables = table1->elementAt(8, 0)->addWidget(std::make_unique<MewStrList>("observables", ""));
        init_states = table1->elementAt(9, 0)->addWidget(std::make_unique<MewStrList>("init_states", ""));

        repeat = table1->elementAt(10, 0)->addWidget(std::make_unique<MewCellIntNum>("repeat", 0));
        step_size = table1->elementAt(11, 0)->addWidget(std::make_unique<MewCellDoubleNum>("step_size", 0.0));
        sequence = table1->elementAt(12, 0)->addWidget(std::make_unique<MewCellString>("sequence",""));
        auto addNumParam= table1->elementAt(13, 0)->addWidget(std::make_unique<Wt::WPushButton>("+ Numerical Param"));
        auto addStrParam= table1->elementAt(14, 0)->addWidget(std::make_unique<Wt::WPushButton>("+ String Param"));

        auto paramTablePanel = table1->elementAt(15, 0)->addWidget(std::make_unique<Wt::WPanel>());
        paramTablePanel->setTitle("sweep_param_info");
        paramTable = paramTablePanel->setCentralWidget(std::make_unique<MewParamTable>());
        paramTable->setAttributeValue("style", "border-spacing: 5px 5px; background-color: #c5fad5");
        addNumParam->clicked().connect([=] {
            int newRow = paramTable->rowCount();
            paramTable->elementAt(newRow, 0)->addWidget(std::make_unique<MewParamConfigCellNum>(*paramTable));
            paramTable->reindexing_table_cells();
        });

        addStrParam->clicked().connect([=] {
            int newRow = paramTable->rowCount();
            paramTable->insertRow(newRow);
            paramTable->elementAt(newRow, 0)->addWidget(std::make_unique<MewParamConfigCellStr>(*paramTable));
            paramTable->reindexing_table_cells();
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

        task_name->set_data(jsonObject["task_name"].toString());
        log_level->set_data(jsonObject["log_level"].toNumber());
        job_slicing_strategy->set_data(jsonObject["log_level"].toString());
        record_propagator->set_data(jsonObject["record_propagator"].toBool());
        record_all_meas->set_data(jsonObject["record_all_meas"].toBool());
        record_density_mat->set_data(jsonObject["record_density_mat"].toBool());
        enable_param_parallel_mode->set_data(jsonObject["enable_param_parallel_mode"].toBool());
        system_dim->set_data(jsonObject["system_dim"].toNumber());
        observables->set_data(jsonObject["observables"]);
        init_states->set_data(jsonObject["init_states"]);
        repeat->set_data(jsonObject["repeat"].toNumber());
        step_size->set_data(jsonObject["step_size"].toNumber());
        sequence->set_data(jsonObject["sequence"].toString());

        paramTable->clear();
        int row_cnt = 0;
        Wt::Json::Array param_list_json = jsonObject["sweep_param_info"];
        for (const auto& param_obj : param_list_json) {
            if (param_obj.type() == Wt::Json::Type::Object) {
                std::cout << "Wt Obj Detected!" << std::endl;
                auto param_obj_con = Wt::Json::Object(param_obj);
                if (param_obj_con.find("string_file") != jsonObject.end()) {
                    std::cout << "Str param Detected!" << std::endl;
                    paramTable->insertRow(row_cnt);
                    MewParamConfigCellStr *newStrCell = paramTable->elementAt(row_cnt, 0)->addWidget(
                            std::make_unique<MewParamConfigCellStr>(*paramTable));
                    newStrCell->load_data(param_obj_con);
                } else if (param_obj_con.find("val_file") != jsonObject.end()) {
                    std::cout << "Num param Detected!" << std::endl;
                    paramTable->insertRow(row_cnt);
                    MewParamConfigCellNum *newNumCell = paramTable->elementAt(row_cnt, 0)->addWidget(
                            std::make_unique<MewParamConfigCellNum>(*paramTable));
                    newNumCell->load_data(param_obj_con);
                }
                row_cnt ++;
            }
        }
        paramTable->reindexing_table_cells();
    }

    void dump_config() {
        Wt::Json::Object sim_config;
        sim_config["task_name"] = Wt::Json::Value(task_name->get_data().second);
        sim_config[log_level->get_data().first] = Wt::Json::Value(log_level->get_data().second);
        sim_config[job_slicing_strategy->get_data().first] = Wt::Json::Value(job_slicing_strategy->get_data().second);
        sim_config[record_propagator->get_data().first] = Wt::Json::Value(record_propagator->get_data().second);
        sim_config[record_all_meas->get_data().first] = Wt::Json::Value(record_all_meas->get_data().second);
        sim_config[record_density_mat->get_data().first] = Wt::Json::Value(record_density_mat->get_data().second);
        sim_config[enable_param_parallel_mode->get_data().first] = Wt::Json::Value(enable_param_parallel_mode->get_data().second);
        sim_config[system_dim->get_data().first] = Wt::Json::Value(system_dim->get_data().second);

        sim_config[observables->get_data().first] = observables->get_data().second;
        sim_config[init_states->get_data().first] = init_states->get_data().second;

        sim_config[repeat->get_data().first] = Wt::Json::Value(repeat->get_data().second);
        sim_config[step_size->get_data().first] = Wt::Json::Value(step_size->get_data().second);
        sim_config[sequence->get_data().first] = Wt::Json::Value(sequence->get_data().second);

        auto paramList = Wt::Json::Array();
        for (int i=0; i<paramTable->rowCount(); i++) {
            if(dynamic_cast<MewParamConfigCellStr*>(paramTable->elementAt(i, 0)->widget(0)) != nullptr) {
                paramList.push_back(((MewParamConfigCellStr*)(paramTable->elementAt(i, 0)->widget(0)))->get_obj());
            } else if (dynamic_cast<MewParamConfigCellNum*>(paramTable->elementAt(i, 0)->widget(0)) != nullptr) {
                paramList.push_back(((MewParamConfigCellStr*)(paramTable->elementAt(i, 0)->widget(0)))->get_obj());
            }
        }
        sim_config["sweep_param_info"] = paramList;

        auto sim_config_rev = reverseEntries(sim_config);

        std::string jsonString = Wt::Json::serialize(sim_config_rev, true);
        std::ofstream file("sim_config.json");
        if (file.is_open()) {
            file << jsonString;
            file.close();
        } else {
            // Error handling
            std::cerr << "Unable to open file for writing." << std::endl;
        }
    }
};




#endif //MYPROJECT_MEWSIMCONFIGPANNEL_H
