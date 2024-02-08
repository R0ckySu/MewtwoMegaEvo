//
// Created by Rocky Su on 24/1/2024.
//

#ifndef MYPROJECT_MEWGATE_H
#define MYPROJECT_MEWGATE_H

#include <Wt/WContainerWidget.h>
#include <Wt/WPushButton.h>
#include <Wt/WTable.h>
#include <Wt/WVBoxLayout.h>
#include <Wt/WHBoxLayout.h>
#include "MewComponents.h"


class BaseGateCell: public Wt::WContainerWidget {
public:
    int cell_index = 0;
};

class MewGateTable: public Wt::WTable {
public:
    void reindexing_table_cells() {
        for (int i=0; i<rowCount(); i++) {
            if(dynamic_cast<BaseGateCell*>(elementAt(i, 0)->widget(0)) != nullptr) {
                BaseGateCell* cell = (BaseGateCell*)elementAt(i, 0)->widget(0);
                cell->cell_index = i;
            }
        }
    }
};

class GateCell : public BaseGateCell {
public:
    MewCellString *tag;
    MewStrList *hamiltonians;
    MewCellDoubleNum *pulse_width;
    MewCellDoubleNum *shift_time;
    MewCellString *ext_shaped_sig_path;

    GateCell(MewGateTable& parent_table) : BaseGateCell() {
        setAttributeValue("style", "background-color: #f96167;");
        auto vLayout = setLayout(std::make_unique<Wt::WVBoxLayout>());
        tag = vLayout->addWidget(std::make_unique<MewCellString>("tag", ""));
        hamiltonians = vLayout->addWidget(std::make_unique<MewStrList>("hamiltonians", ""));
        pulse_width = vLayout->addWidget(std::make_unique<MewCellDoubleNum>("pulse_width", 0.0));
        shift_time = vLayout->addWidget(std::make_unique<MewCellDoubleNum>("shift_time", 0.0));
        ext_shaped_sig_path = vLayout->addWidget(std::make_unique<MewCellString>("ext_shaped_sig_path", ""));
        auto deleteButton = vLayout->addWidget(std::make_unique<Wt::WPushButton>("Delete"));
        deleteButton->clicked().connect([=, &parent_table] {
            parent_table.removeRow(cell_index);
            parent_table.reindexing_table_cells();
        });
    }

    void loadInfoFromJson(Wt::Json::Object dict) {
        tag->set_data(dict.get(std::string(tag->get_data().first)));
        hamiltonians->set_data(dict.get(std::string(hamiltonians->get_data().first)));
        pulse_width->set_data(dict.get(std::string(pulse_width->get_data().first)));
        shift_time->set_data(dict.get(std::string(shift_time->get_data().first)));
        ext_shaped_sig_path->set_data(dict.get(std::string(ext_shaped_sig_path->get_data().first)));
    }

    Wt::Json::Object getJsonObj() {
        Wt::Json::Object g = Wt::Json::Object();
        g[tag->get_data().first] = Wt::Json::Value(tag->get_data().second);
        g[hamiltonians->get_data().first] = Wt::Json::Value(hamiltonians->get_data().second);
        g[pulse_width->get_data().first] = Wt::Json::Value(pulse_width->get_data().second);
        g[shift_time->get_data().first] = Wt::Json::Value(shift_time->get_data().second);
        return g;
    }
};



class MewGateConfig: public Wt::WContainerWidget {
public:
    MewGateTable* table;
    MewGateConfig() : Wt::WContainerWidget() {
        setAttributeValue("style", "background-color: #fce77d;");
        auto saveButton = addWidget(std::make_unique<Wt::WPushButton>("Save File"));
        saveButton->clicked().connect([=] {
            dump_config();
        });

        auto buttonContainer= addWidget(std::make_unique<Wt::WContainerWidget>());;
        auto hButtonLayout = buttonContainer->setLayout(std::make_unique<Wt::WHBoxLayout>());
        auto addButtonGate= hButtonLayout->addWidget(std::make_unique<Wt::WPushButton>("+ Gate"));
        addButtonGate->setAttributeValue("style", "background-color: #f96167;");

        table = addWidget(std::make_unique<MewGateTable>());
        table->setAttributeValue("style", "border-collapse: separate; border-spacing: 5px 10px;");

        addButtonGate->clicked().connect([=] {
            int newRow = table->rowCount();
            table->insertRow(newRow);
            table->elementAt(newRow, 0)->addWidget(std::make_unique<GateCell>(*table));
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

        Wt::Json::Array G_list_json = jsonObject["gate_defs"];
        table->clear();
        int row_cnt = 0;
        for (const auto& g_obj : G_list_json) {
            if (g_obj.type() == Wt::Json::Type::Object) {
                auto g_obj_s = Wt::Json::Object(g_obj);
                table->insertRow(row_cnt);
                GateCell *newGateCell = table->elementAt(row_cnt, 0)->addWidget(
                        std::make_unique<GateCell>(*table));
                newGateCell->loadInfoFromJson(g_obj_s);
                row_cnt ++;
            }
        }
        table->reindexing_table_cells();
    }

    void dump_config() {
        Wt::Json::Object gate_config;

        auto gate_List = Wt::Json::Array();
        for (int i=0; i<table->rowCount(); i++) {
            if(dynamic_cast<GateCell*>(table->elementAt(i, 0)->widget(0)) != nullptr) {
                gate_List.push_back(((GateCell*)(table->elementAt(i, 0)->widget(0)))->getJsonObj());
            }
        }
        gate_config["gate_defs"] = gate_List;

        std::string jsonString = Wt::Json::serialize(gate_config, true);
        std::ofstream file("gate_config.json");
        if (file.is_open()) {
            file << jsonString;
            file.close();
        } else {
            // Error handling
            std::cerr << "Unable to open file for writing." << std::endl;
        }
    }
};

#endif //MYPROJECT_MEWGATE_H
