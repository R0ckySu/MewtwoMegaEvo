//
// Created by Rocky Su on 24/1/2024.
//

#ifndef MYPROJECT_MEWGATE_H
#define MYPROJECT_MEWGATE_H

#include <Wt/WContainerWidget.h>
#include <Wt/WTable.h>
#include "MewComponents.h"

class BaseGateCell: public Wt::WContainerWidget {
public:
    int cell_index = 0;
};

class MewGateTable: public Wt::WTable {
public:
    void reindexing_table_cells();
};

class GateCell : public BaseGateCell {
public:
    MewCellString *tag;
    MewStrList *hamiltonians;
    MewCellDoubleNum *pulse_width;
    MewCellDoubleNum *shift_time;
    MewCellString *ext_shaped_sig_path;

    GateCell(MewGateTable& parent_table);
    void loadInfoFromJson(Wt::Json::Object dict);
    Wt::Json::Object getJsonObj();
};

class MewGateConfig: public Wt::WContainerWidget {
public:
    MewGateTable *table;
    MewGateConfig();
    void load_from_file(std::string filePath);
    void dump_config();
};

#endif //MYPROJECT_MEWGATE_H
