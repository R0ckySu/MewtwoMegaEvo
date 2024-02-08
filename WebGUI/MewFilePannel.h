//
// Created by Rocky Su on 9/2/2024.
//

#ifndef MYPROJECT_MEWFILEPANNEL_H
#define MYPROJECT_MEWFILEPANNEL_H


class MewFilePannel {

};

//
//#include <Wt/WApplication.h>
//#include <Wt/WPushButton.h>
//#include <Wt/WTable.h>
//#include <Wt/WText.h>
//#include <Wt/WDialog.h>
//#include <Wt/WTextArea.h>
//#include <filesystem>
//#include <fstream>
//#include <sstream>
//
//namespace fs = std::filesystem;
//
//class FileBrowserApp : public Wt::WApplication {
//public:
//    Wt::WTable* table;
//    std::string directoryPath;
//
//    FileBrowserApp(const Wt::WEnvironment& env, const std::string& dirPath)
//            : Wt::WApplication(env), directoryPath(dirPath) {
//        setTitle("Text File Browser with Delete Option");
//
//        table = root()->addWidget(std::make_unique<Wt::WTable>());
//        table->setHeaderCount(1);
//        table->elementAt(0, 0)->addWidget(std::make_unique<Wt::WText>("File Name"));
//        table->elementAt(0, 1)->addWidget(std::make_unique<Wt::WText>("Actions"));
//
//        listTextFiles();
//    }
//
//    void listTextFiles() {
//        int row = 1; // Start from the first row after the header
//        for (const auto& entry : fs::directory_iterator(directoryPath)) {
//            if (entry.is_regular_file()) {
//                auto fileName = entry.path().filename().string();
//
//                table->elementAt(row, 0)->addWidget(std::make_unique<Wt::WText>(fileName));
//
//                auto viewButton = std::make_unique<Wt::WPushButton>("View");
//                viewButton->clicked().connect([=] {
//                    displayFileContent(entry.path());
//                });
//                table->elementAt(row, 1)->addWidget(std::move(viewButton));
//
//                auto deleteButton = std::make_unique<Wt::WPushButton>("Delete");
//                deleteButton->clicked().connect([=] {
//                    fs::remove(entry.path());
//                    table->removeRow(row); // Remove the row from the table
//                });
//                table->elementAt(row, 1)->addWidget(std::move(deleteButton));
//
//                ++row;
//            }
//        }
//    }
//
//    void displayFileContent(const fs::path& filePath) {
//        std::ifstream file(filePath);
//        std::stringstream buffer;
//        buffer << file.rdbuf();
//        file.close(); // Close the file after reading its content
//
//        auto dialog = root()->addChild(std::make_unique<Wt::WDialog>("Edit File"));
//        auto textArea = dialog->contents()->addWidget(std::make_unique<Wt::WTextArea>());
//        textArea->setText(buffer.str());
//        textArea->resize(400, 300); // Adjust size as needed
//
//        // Save Button
//        auto saveButton = dialog->footer()->addWidget(std::make_unique<Wt::WPushButton>("Save"));
//        saveButton->clicked().connect([=] {
//            std::ofstream outFile(filePath);
//            if (outFile.is_open()) {
//                outFile << textArea->text();
//                outFile.close();
//                dialog->accept(); // Close the dialog after saving
//            } else {
//                // Error handling: could not open file for writing
//                std::cerr << "Error: Unable to open file for writing." << std::endl;
//            }
//        });
//
//        // Close Button
//        auto closeButton = dialog->footer()->addWidget(std::make_unique<Wt::WPushButton>("Close"));
//        closeButton->clicked().connect([=] { dialog->reject(); });
//
//        dialog->finished().connect([=] { root()->removeChild(dialog); });
//        dialog->show();
//    }
//};
//
//int main(int argc, char **argv) {
//    return Wt::WRun(argc, argv, [&](const Wt::WEnvironment& env) {
//        // Specify the directory path where the text files are located
//        std::string directoryPath = "/Users/rockysu/CodeRepo/MewtwoMegaEvo.git/Playground/config_files";
//        return std::make_unique<FileBrowserApp>(env, directoryPath);
//    });
//}


#endif //MYPROJECT_MEWFILEPANNEL_H
