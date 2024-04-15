//
// Created by Rocky Su on 4/3/2024.
// GPT4 assisted

#ifndef MEWTWOMEGAEVO_MEWUSERINVENTORYAPP_H
#define MEWTWOMEGAEVO_MEWUSERINVENTORYAPP_H

#include <Wt/WApplication.h>
#include <Wt/WEnvironment.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WTable.h>
#include <Wt/WPushButton.h>
#include <Wt/WText.h>
#include <filesystem>
#include <string>
#include <Wt/WFileResource.h>
#include <cstdlib>

#define DemoConfigFolder "./Demo_configs"

class MewUserInventoryApp : public Wt::WApplication {
public:
    MewUserInventoryApp(const Wt::WEnvironment& env)
            : Wt::WApplication(env) {
        userID = env.getParameter("uid") ? *env.getParameter("uid") : "defaultUserId";
        createDirectoryIfNotExists(std::filesystem::path(userID));
        userConfigFolder = std::filesystem::path(userID) / "config_files";
        auto userResultsFolder = std::filesystem::path(userID) / "sim_results";
        setTitle("User Inventory");
        useStyleSheet("auth2/css/style.css");
        listDemoConfigs(DemoConfigFolder);
        listSimResults(userResultsFolder.string());
    }

private:
    std::string userID;
    std::filesystem::path userConfigFolder;
    void listDemoConfigs(const std::string& directoryPath) {
        namespace fs = std::filesystem;

        auto container = root()->addWidget(std::make_unique<Wt::WContainerWidget>());
        auto table = container->addWidget(std::make_unique<Wt::WTable>());
        table->setHeaderCount(1);
        table->elementAt(0, 0)->addWidget(std::make_unique<Wt::WText>("Demo Name"));
        table->elementAt(0, 1)->addWidget(std::make_unique<Wt::WText>("Actions"));

        table->addStyleClass("table table-striped");
        table->setWidth(Wt::WLength("50%"));

        int row = 1;
        try {
            for (const auto& entry : fs::directory_iterator(directoryPath)) {
                if (entry.is_directory()) {
                    std::string folderName = entry.path().filename().string();
                    table->elementAt(row, 0)->addWidget(std::make_unique<Wt::WText>(folderName));

                    // Add a button for clearing and copying
                    auto copyBtn = std::make_unique<Wt::WPushButton>("Copy as template");
                    copyBtn->clicked().connect([=] {
                        clearAndCopyFolder(entry.path().string(), this->userConfigFolder);
                        Wt::WApplication::instance()->redirect(std::string("/simconfig?uid=").append(userID));
                    });
                    table->elementAt(row, 1)->addWidget(std::move(copyBtn));

                    ++row;
                }
            }
        } catch (const fs::filesystem_error& e) {
            container->addWidget(std::make_unique<Wt::WText>("Error accessing directory: " + std::string(e.what())));
        }
    }

    bool zipFolder(const std::string& sourceFolder, const std::string& outputZipFile) {
        std::string command = "zip -r " + outputZipFile + " " + sourceFolder;
        return std::system(command.c_str()) == 0; // Check the return value, 0 means success
    }

    void listSimResults(const std::string& directoryPath) {
        namespace fs = std::filesystem;

        auto container = root()->addWidget(std::make_unique<Wt::WContainerWidget>());
        auto table = container->addWidget(std::make_unique<Wt::WTable>());
        table->setHeaderCount(1);
        table->elementAt(0, 0)->addWidget(std::make_unique<Wt::WText>("Result Name"));
        table->elementAt(0, 1)->addWidget(std::make_unique<Wt::WText>("Actions"));

        table->addStyleClass("table table-striped");
        table->setWidth(Wt::WLength("50%"));

        int row = 1;
        try {
            for (const auto& entry : fs::directory_iterator(directoryPath)) {
                if (entry.is_directory()) {
                    std::string folderName = entry.path().filename().string();
                    table->elementAt(row, 0)->addWidget(std::make_unique<Wt::WText>(folderName));

                    // Add a button for clearing and copying
                    auto downloadBtn = std::make_unique<Wt::WPushButton>("Download Results");
                    downloadBtn->clicked().connect([=] {
//                        clearAndCopyFolder(entry.path().string(), this->userConfigFolder);
//                        Wt::WApplication::instance()->redirect(std::string("/simconfig?uid=").append(userID));
                    });
                    table->elementAt(row, 1)->addWidget(std::move(downloadBtn));

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

    void createDirectoryIfNotExists(const std::filesystem::path& path) {
        // Check if the directory exists
        if (!std::filesystem::exists(path)) {
            // Attempt to create the directory
            if (std::filesystem::create_directory(path)) {
                std::cout << "Directory created: " << path << std::endl;
                std::filesystem::create_directory(path / "sim_results");
                std::filesystem::create_directory(path / "config_files");
            } else {
                std::cout << "Failed to create directory: " << path << std::endl;
            }
        } else {
            std::cout << "Directory already exists: " << path << std::endl;
        }
    }
};


#endif //MEWTWOMEGAEVO_MEWUSERINVENTORYAPP_H
