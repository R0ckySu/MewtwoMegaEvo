//
// Created by Rocky Su on 4/3/2024.
// GPT4 assisted

#ifndef MEWTWOMEGAEVO_MEWUSERINVENTORYAPP_H
#define MEWTWOMEGAEVO_MEWUSERINVENTORYAPP_H

#include <Wt/WApplication.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WTable.h>
#include <Wt/WPushButton.h>
#include <Wt/WText.h>
#include <filesystem>
#include <string>


class MewUserInventoryApp : public Wt::WApplication {
public:
    MewUserInventoryApp(const Wt::WEnvironment& env,
                        const std::string& directoryPath,
                        const std::string& destinationPath)
            : Wt::WApplication(env), destinationPath(destinationPath) {
        setTitle("User Inventory");
        listDirectories(directoryPath);
    }

private:
    std::string destinationPath;
    void listDirectories(const std::string& directoryPath) {
        namespace fs = std::filesystem;

        auto container = root()->addWidget(std::make_unique<Wt::WContainerWidget>());
        auto table = container->addWidget(std::make_unique<Wt::WTable>());
        table->setHeaderCount(1);
        table->elementAt(0, 0)->addWidget(std::make_unique<Wt::WText>("Subdirectory Name"));
        table->elementAt(0, 1)->addWidget(std::make_unique<Wt::WText>("Actions"));

        table->addStyleClass("table table-striped");
        table->setWidth(Wt::WLength("100%"));

        int row = 1;
        try {
            for (const auto& entry : fs::directory_iterator(directoryPath)) {
                if (entry.is_directory()) {
                    std::string folderName = entry.path().filename().string();
                    table->elementAt(row, 0)->addWidget(std::make_unique<Wt::WText>(folderName));

                    // Add a button for clearing and copying
                    auto copyBtn = std::make_unique<Wt::WPushButton>("Copy");
                    copyBtn->clicked().connect([=] {
                        clearAndCopyFolder(entry.path().string(), this->destinationPath);
                    });
                    table->elementAt(row, 1)->addWidget(std::move(copyBtn));

                    ++row;
                }
            }
        } catch (const fs::filesystem_error& e) {
            container->addWidget(std::make_unique<Wt::WText>("Error accessing directory: " + std::string(e.what())));
        }
    }

    void clearAndCopyFolder(const std::string& sourcePath, const std::string& destPath) {
        namespace fs = std::filesystem;
        try {
            // Clear destination directory
            for (const auto& entry : fs::directory_iterator(destPath)) {
                fs::remove_all(entry.path());
            }

            // Copy all from source to destination
            fs::copy(sourcePath, destPath, fs::copy_options::recursive);
            Wt::log("info") << "Successfully copied " << sourcePath << " to " << destPath;
        } catch (const fs::filesystem_error& e) {
            Wt::log("error") << "Error during copy: " << e.what();
        }
    }
};


#endif //MEWTWOMEGAEVO_MEWUSERINVENTORYAPP_H
