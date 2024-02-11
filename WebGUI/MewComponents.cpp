//
// Created by Rocky Su on 26/1/2024.
//

#include "MewComponents.h"
#include <Wt/Json/Object.h>
#include <Wt/Json/Array.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WLineEdit.h>
#include <Wt/WVBoxLayout.h>
#include <Wt/WHBoxLayout.h>
#include <Wt/WComboBox.h>
#include <Wt/WPanel.h>
#include <Wt/WCheckBox.h>
#include <Wt/WIntValidator.h>
#include <Wt/WDoubleValidator.h>
#include <Wt/Json/Value.h>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>

std::string DoubleToString(double value) {
    std::ostringstream stream;
    stream << std::scientific << std::setprecision(std::numeric_limits<double>::digits10) << value;

    std::string str = stream.str();

    // Remove trailing zeros in the fractional part
    auto decimalPos = str.find('.');
    if (decimalPos != std::string::npos) {
        auto ePos = str.find('e', decimalPos);
        auto lastNonZeroPos = str.find_last_not_of('0', ePos - 1);
        if (lastNonZeroPos == decimalPos) {
            // If the last non-zero character is the decimal point, remove it along with trailing zeros
            str.erase(decimalPos, ePos - decimalPos);
        } else {
            // Otherwise, just remove the trailing zeros
            str.erase(lastNonZeroPos + 1, ePos - lastNonZeroPos - 1);
        }
    }

    return str;
}

MewCellString::MewCellString(std::string title, std::string content) {
    titleField = addWidget(std::make_unique<Wt::WText>(title));
    contentField = addWidget(std::make_unique<Wt::WLineEdit>(content));
    contentField->setMargin(5, Wt::Side::Left);
}

std::pair<std::string , std::string> MewCellString::get_data() {
    return std::make_pair(std::string(titleField->text().toUTF8()),std::string(contentField->text().toUTF8()));
}

void MewCellString::set_data(std::string str) {
    this->contentField->setText(str);
}

MewCellBool::MewCellBool(std::string title, bool tick) {
    titleField = addWidget(std::make_unique<Wt::WText>(title));
    tickBox = addWidget(std::make_unique<Wt::WCheckBox>());
    tickBox->setMargin(5, Wt::Side::Left);
    if(tick) {
        tickBox->setCheckState(Wt::CheckState(Wt::CheckState::Checked));
    } else {
        tickBox->setCheckState(Wt::CheckState(Wt::CheckState::Unchecked));
    }
}

std::pair<std::string, bool> MewCellBool::get_data() {
    bool check = false;
    if(tickBox->checkState() == Wt::CheckState::Checked) {
        check = true;
    }
    return std::pair(titleField->text().toUTF8(), check);
}

void MewCellBool::set_data(bool check) {
    if(check) {
        tickBox->setCheckState(Wt::CheckState::Checked);
    } else {
        tickBox->setCheckState(Wt::CheckState::Unchecked);
    }
}

MewCellIntNum::MewCellIntNum(std::string title, int num) {
    titleField = addWidget(std::make_unique<Wt::WText>(title));
    num_contentField = addWidget(std::make_unique<Wt::WLineEdit>(std::to_string(num)));
    num_contentField->setMargin(5, Wt::Side::Left);
    num_contentField->setValidator(std::make_shared<Wt::WIntValidator>());
}

void MewCellIntNum::set_data(int num) {
    num_contentField->setText(std::to_string(num));
}

std::pair<std::string, int> MewCellIntNum::get_data() {
//    double num = std::string(num_contentField->text().toUTF8());
    return std::make_pair<std::string, int>(titleField->text().toUTF8(), std::atoi(num_contentField->text().toUTF8().c_str()));
}

MewCellDoubleNum::MewCellDoubleNum(std::string title, double num) {
    titleField = addWidget(std::make_unique<Wt::WText>(title));
    num_contentField = addWidget(std::make_unique<Wt::WLineEdit>(DoubleToString(num)));
    num_contentField->setMargin(5, Wt::Side::Left);
    num_contentField->setValidator(std::make_shared<Wt::WDoubleValidator>());
}

void MewCellDoubleNum::set_data(double num) {
    num_contentField->setText(DoubleToString(num));
}

std::pair<std::string, double> MewCellDoubleNum::get_data() {
    return std::make_pair<std::string, double>(titleField->text().toUTF8(), std::atof(num_contentField->text().toUTF8().c_str()));
}


MewOptions::MewOptions(std::string title, std::vector<std::string> options) {
    titleField = addWidget(std::make_unique<Wt::WText>(title));
    all_options = options;
    auto dd = std::make_unique<Wt::WComboBox>();
    for(int i=0; i < all_options.size(); i++) {
        dd->addItem(Wt::WString(all_options.at(i)));
    }
    dd->setMargin(5, Wt::Side::Left);
    dropdown = addWidget(std::move(dd));
}

void MewOptions::set_data(std::string option_item) {
    auto it = std::find(all_options.begin(), all_options.end(), option_item);
    if (it == all_options.end()) {
        // name not in vector
        printf("Option not found!");
    } else {
        auto index = std::distance(all_options.begin(), it);
        dropdown->setCurrentIndex(index);
    }
}

std::pair<std::string, std::string> MewOptions::get_data() {
    return std::pair<std::string, std::string>(titleField->text().toUTF8(), dropdown->currentText().toUTF8());
}

MewStrList::MewStrList(std::string title, std::string item_list_string) {
    titleField = addWidget(std::make_unique<Wt::WText>(title));
    contentField = addWidget(std::make_unique<Wt::WLineEdit>(item_list_string));
    contentField->setMargin(5, Wt::Side::Left);
}

void MewStrList::set_data(Wt::Json::Array str_list) {
    std::string result;
    for (size_t i = 0; i < str_list.size(); ++i) {
        if (i != 0) {
            result += ",";
        }
        Wt::Json::Value value = str_list[i];
        if (value.type() == Wt::Json::Type::String) {
            result.append(value.toString());
        }
    }
    contentField->setText(result);
}

Wt::Json::Array commaSeparatedStringToJsonArray(const std::string& input) {
    Wt::Json::Array jsonArray;
    std::stringstream ss(input);
    std::string item;
    while (std::getline(ss, item, ',')) {
        jsonArray.push_back(Wt::Json::Value(item));
    }
    return jsonArray;
}

std::pair<std::string, Wt::Json::Array> MewStrList::get_data() {
    Wt::Json::Array jsonArray = commaSeparatedStringToJsonArray(contentField->text().toUTF8());
    return std::pair<std::string, Wt::Json::Array>(titleField->text().toUTF8(), jsonArray);
}

//MewObjPannel::MewObjPannel(std::string title, Wt::Json::Object config_json) {
//    this->setTitle(title);
//    this->addStyleClass("centered-example");
//    this->setCollapsible(true);
//
//    Wt::WAnimation animation(Wt::AnimationEffect::SlideInFromTop,
//                             Wt::TimingFunction::EaseOut,
//                             100);
//    this->setAnimation(animation);
//    auto container = std::make_unique<Wt::WContainerWidget>();
//    container->setStyleClass("yellow-box");
//    auto hbox = container->setLayout(std::make_unique<Wt::WVBoxLayout>());
////    widget_LUT = std::map<std::string, Wt::WContainerWidget>();
//
////    auto keys = std::make_unique<Wt::Json::Array>(config_json.get("key_order"));
//    this->key_order = config_json.get("key_order");
////    for(int i, i <=keys. )
//    Wt::Json::Array::iterator it;
//    for (it = key_order.begin(); it != key_order.end(); ++it) {
//        std::string key = *it;
//        auto value = config_json.get(key);
//        switch (value.type()) {
//            case Wt::Json::Type::Bool :
//                widget_LUT.insert(std::make_pair(key, hbox->addWidget(std::make_unique<MewCellBool>(key, value))));
//                break;
//            case Wt::Json::Type::String :
//                widget_LUT.insert(std::make_pair(key, hbox->addWidget(std::make_unique<MewCellString>(key, value))));
//                break;
//            case Wt::Json::Type::Number :
//                widget_LUT.insert(std::make_pair(key, hbox->addWidget(std::make_unique<MewCellNum>(key, value))));
//                break;
//            case Wt::Json::Type::Array :
//                widget_LUT.insert(std::make_pair(key, hbox->addWidget(std::make_unique<MewListPannel>(key, value))));
//                break;
//            case Wt::Json::Type::Object :
//                widget_LUT.insert(std::make_pair(key, hbox->addWidget(std::make_unique<MewObjPannel>(key, value))));
//                break;
//            default:
//                break;
//        }
//    }
//    this->setCentralWidget(std::move(container));
//}
//
//MewListPannel::MewListPannel(std::string title, Wt::Json::Array config_array) {
//    this->setTitle(title);
//    this->addStyleClass("centered-example");
//    this->setCollapsible(true);
//
//    Wt::WAnimation animation(Wt::AnimationEffect::SlideInFromTop,
//                             Wt::TimingFunction::EaseOut,
//                             100);
//    this->setAnimation(animation);
//    auto container = std::make_unique<Wt::WContainerWidget>();
//    container->setStyleClass("yellow-box");
//    auto hbox = container->setLayout(std::make_unique<Wt::WVBoxLayout>());
//
//    for (auto val : config_array) {
//        switch (val.type()) {
//            case Wt::Json::Type::String :
//                hbox->addWidget(std::make_unique<Wt::WLineEdit>(val));
//                break;
//            case Wt::Json::Type::Number :
//                hbox->addWidget(std::make_unique<Wt::WLineEdit>(std::to_string(double(val))));
//                break;
//            case Wt::Json::Type::Object :
//                hbox->addWidget(std::make_unique<MewObjPannel>("", val));
//                break;
//            default:
//                break;
//        }
//    }
//    this->setCentralWidget(std::move(container));
//}