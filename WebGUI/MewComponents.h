//
// Created by Rocky Su on 26/1/2024.
//

#ifndef MYPROJECT_MEWCOMPONENTS_H
#define MYPROJECT_MEWCOMPONENTS_H

#include <Wt/WPanel.h>
#include <Wt/WText.h>
#include <Wt/Json/Object.h>
#include <Wt/WContainerWidget.h>
#include <Wt/Json/Array.h>
#include <Wt/WLineEdit.h>

class MewCellString: public Wt::WContainerWidget {
public:
    MewCellString(std::string title, std::string content);
    void set_data(std::string str);
    std::pair<std::string , std::string> get_data();
private:
    Wt::WText *titleField;
    Wt::WLineEdit *contentField;
};

class MewCellBool: public Wt::WContainerWidget {
public:
    MewCellBool(std::string title, bool tick);
    void set_data(bool check);
    std::pair<std::string, bool> get_data();
private:
    Wt::WText *titleField;
    Wt::WCheckBox *tickBox;
};

class MewCellIntNum: public Wt::WContainerWidget {
public:
    MewCellIntNum(std::string title, int num);
    void set_data(int num);
    std::pair<std::string, int> get_data();
private:
    Wt::WText *titleField;
    Wt::WLineEdit *num_contentField;
};

class MewCellDoubleNum: public Wt::WContainerWidget {
public:
    MewCellDoubleNum(std::string title, double num);
    void set_data(double num);
    std::pair<std::string, double> get_data();
private:
    Wt::WText *titleField;
    Wt::WLineEdit *num_contentField;
};

class MewOptions: public Wt::WContainerWidget {
public:
    MewOptions(std::string title, std::vector<std::string> options);
    void set_data(std::string option_item);
    std::pair<std::string, std::string> get_data();
private:
    std::vector<std::string> all_options;
    Wt::WText *titleField;
    Wt::WComboBox *dropdown;
};

class MewStrList: public Wt::WContainerWidget {
public:
    MewStrList(std::string title, std::string item_list_string);
    void set_data(Wt::Json::Array str_list);
    std::pair<std::string, Wt::Json::Array>get_data();
private:
    Wt::WText *titleField;
    Wt::WLineEdit *contentField;
};

//class MewListPannel: public Wt::WPanel {
//public:
//    MewListPannel(std::string title, Wt::Json::Array config_array);
//    std::map<std::string, Wt::WWidget*> widget_LUT;
////    std::pair<std::string, Wt::Json::Array> get_data();
//private:
//    Wt::Json::Array additive_template;
//};

//class MewObjPannel: public Wt::WPanel {
//public:
//    MewObjPannel(std::string title, Wt::Json::Object config_json);
//    std::map<std::string, Wt::WWidget*> widget_LUT;
////    std::pair<std::string, Wt::Json::Object> get_data();
//
//private:
//    Wt::Json::Array key_order;
//};

#endif //MYPROJECT_MEWCOMPONENTS_H
