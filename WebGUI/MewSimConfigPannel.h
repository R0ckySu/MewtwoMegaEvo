//
// Created by Rocky Su on 3/6/2023.
//

#ifndef MYPROJECT_MEWSIMCONFIGPANNEL_H
#define MYPROJECT_MEWSIMCONFIGPANNEL_H
//#include <iostream>
#include "MewComponents.h"
#include <Wt/WTable.h>

class MewParamConfigCell : public Wt::WContainerWidget {
public:
    int cell_index;
};

class MewParamTable: public Wt::WTable {
public:
    void reindexing_table_cells();
};

class MewParamConfigCellStr : public MewParamConfigCell {
public:
    MewCellString *class_type;
    MewCellString *tag;
    MewCellString *property;
    MewCellString *string_file;
    MewParamConfigCellStr(MewParamTable& parent_table);
    void load_data(Wt::Json::Object data);
    Wt::Json::Object get_obj();
};

class MewParamConfigCellNum : public MewParamConfigCell {
public:
    MewCellString *class_type;
    MewCellString *tag;
    MewCellString *property;
    MewCellString *val_file;
    MewParamConfigCellNum(MewParamTable& parent_table);
    void load_data(Wt::Json::Object data);
    Wt::Json::Object get_obj();
};

class MewSimConfigPannel : public Wt::WContainerWidget {
public:
    MewCellString *task_name;
    MewCellIntNum *log_level;
    MewOptions *job_slicing_strategy;
    MewCellBool *record_propagator;
    MewCellBool *record_all_meas;
    MewCellBool *record_density_mat;
    MewCellBool *reset_rotating_frame;
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

    MewSimConfigPannel(Wt::WContainerWidget* parent = nullptr);
    void load_from_file(std::string filePath);
    void dump_config();
};




#endif //MYPROJECT_MEWSIMCONFIGPANNEL_H
