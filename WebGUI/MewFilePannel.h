//
// Created by Rocky Su on 9/2/2024.
//

#ifndef MYPROJECT_MEWFILEPANNEL_H
#define MYPROJECT_MEWFILEPANNEL_H

#include <Wt/WPushButton.h>
#include <Wt/WTable.h>
#include <Wt/WText.h>
#include <Wt/WLineEdit.h>
#include <Wt/WDialog.h>
#include <Wt/WTextArea.h>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

class MewFilePannel: public Wt::WContainerWidget {
public:
    Wt::WTable* table;
    std::string directoryPath;

    MewFilePannel(std::string dirPath): Wt::WContainerWidget() {
        directoryPath = dirPath;
        table = addWidget(std::make_unique<Wt::WTable>());
        auto createFileButton = addWidget(std::make_unique<Wt::WPushButton>("Create New File"));
        createFileButton->clicked().connect([=] {
            createNewFileDialog();
        });
        listTextFiles();
    }

    void createNewFileDialog() {
        auto dialog = addChild(std::make_unique<Wt::WDialog>("Create New File"));

        auto fileNameEdit = dialog->contents()->addWidget(std::make_unique<Wt::WLineEdit>());
        fileNameEdit->setPlaceholderText("Enter file name here...");

        auto textArea = dialog->contents()->addWidget(std::make_unique<Wt::WTextArea>());
        textArea->setPlaceholderText("Enter file content here...");
        textArea->resize(400, 300); // Adjust size as needed

        // Save Button
        auto saveButton = dialog->footer()->addWidget(std::make_unique<Wt::WPushButton>("Save"));
        saveButton->clicked().connect([=] {
            std::string fileName = fileNameEdit->text().toUTF8();
            if (!fileName.empty()) {
                std::ofstream outFile(directoryPath + "/" + fileName);
                if (outFile.is_open()) {
                    outFile << textArea->text().toUTF8();
                    outFile.close();
                    dialog->accept(); // Close the dialog after saving
                    // Optional: Update the file list to include the new file
                    listTextFiles(); // Assuming this function reloads the file list
                } else {
                    // Error handling: could not open file for writing
                    std::cerr << "Error: Unable to open file for writing." << std::endl;
                }
            }
        });

        // Close Button
        auto closeButton = dialog->footer()->addWidget(std::make_unique<Wt::WPushButton>("Close"));
        closeButton->clicked().connect([=] { dialog->reject(); });

        dialog->finished().connect([=] {removeChild(dialog); });
        dialog->show();
    }

    void listTextFiles() {
        table->clear();
        table->setHeaderCount(1);
        table->elementAt(0, 0)->addWidget(std::make_unique<Wt::WText>("File Name"));
        table->elementAt(0, 1)->addWidget(std::make_unique<Wt::WText>("Actions"));
        int row = 1; // Start from the first row after the header
        for (const auto& entry : fs::directory_iterator(directoryPath)) {
            if (entry.is_regular_file()) {
                auto filePath = entry.path();
                if (filePath.extension() != ".json") { // Exclude .json files
                    auto fileName = filePath.filename().string();

                    table->elementAt(row, 0)->addWidget(std::make_unique<Wt::WText>(fileName));

                    auto viewButton = std::make_unique<Wt::WPushButton>("View");
                    viewButton->clicked().connect([=] {
                        displayFileContent(entry.path());
                    });
                    table->elementAt(row, 1)->addWidget(std::move(viewButton));

                    auto deleteButton = std::make_unique<Wt::WPushButton>("Delete");
                    deleteButton->clicked().connect([=] {
                        fs::remove(entry.path());
                        table->removeRow(row); // Remove the row from the table
                        // Note: Consider refreshing the entire table to maintain correct row indexing
                    });
                    table->elementAt(row, 1)->addWidget(std::move(deleteButton));
                    ++row;
                }
            }
        }
    }

    void displayFileContent(const fs::path& filePath) {
        std::ifstream file(filePath);
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close(); // Close the file after reading its content

        auto dialog = addChild(std::make_unique<Wt::WDialog>("Edit File"));
        auto textArea = dialog->contents()->addWidget(std::make_unique<Wt::WTextArea>());
        textArea->setText(buffer.str());
        textArea->resize(600, 300); // Adjust size as needed

        // Save Button
        auto saveButton = dialog->footer()->addWidget(std::make_unique<Wt::WPushButton>("Save"));
        saveButton->clicked().connect([=] {
            std::ofstream outFile(filePath);
            if (outFile.is_open()) {
                outFile << textArea->text();
                outFile.close();
                dialog->accept(); // Close the dialog after saving
            } else {
                // Error handling: could not open file for writing
                std::cerr << "Error: Unable to open file for writing." << std::endl;
            }
        });

        // Close Button
        auto closeButton = dialog->footer()->addWidget(std::make_unique<Wt::WPushButton>("Close"));
        closeButton->clicked().connect([=] { dialog->reject(); });

        dialog->finished().connect([=] {removeChild(dialog);});
        dialog->show();
    }

};

#endif //MYPROJECT_MEWFILEPANNEL_H
