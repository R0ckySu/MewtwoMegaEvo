//
// Created by Rocky Su on 9/2/2024.
//

#ifndef MEWTWOMEGAEVO_MEWLAUNCHPAD_H
#define MEWTWOMEGAEVO_MEWLAUNCHPAD_H

#include <Wt/WContainerWidget.h>
#include <Wt/WTextArea.h>
#include <Wt/WServer.h>
#include <Wt/WApplication.h>
#include <Wt/WPushButton.h>
#include <Wt/WVBoxLayout.h>
#include <Wt/WHBoxLayout.h>
#include <Wt/WTimer.h>
#include <Wt/WProgressBar.h>
#include <thread>
#include <future>
#include <iostream>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

#define command_path "./MewtwoMegaEvo"

class TaskLaunchDelegate {
public:
    std::string task_name;
    virtual void willLaunchSimulator() = 0;
    virtual void didLaunchSimulator() = 0;
};

class MewLaunchPad: public Wt::WContainerWidget{
public:
    std::string working_folder;
    std::string export_folder;
    std::string current_time_stamp_string="";
    std::string current_task_name="";
    Wt::WTimer* progress_bar_timer;
    Wt::WPushButton* Lauchbutton;
    Wt::WPushButton* Downloadbutton;
    std::shared_ptr<Wt::WResource> zipFileResource;
    TaskLaunchDelegate* delegate;
    std::string most_recent_job_time_stamp;

    MewLaunchPad() : Wt::WContainerWidget() {
        auto vLayout = this->setLayout(std::make_unique<Wt::WVBoxLayout>());
        auto outputText = vLayout->addWidget(std::make_unique<Wt::WTextArea>());
        outputText->setReadOnly(true); // Make it read-only if editing is not required
//        outputText->resize(500, 200);

        auto buttonContainer = vLayout->addWidget(std::make_unique<Wt::WContainerWidget>());
        auto buttonHLayout = buttonContainer->setLayout(std::make_unique<Wt::WHBoxLayout>());
        Lauchbutton = buttonHLayout->addWidget(std::make_unique<Wt::WPushButton>("Launch Simulator!"));

        auto progressBar = vLayout->addWidget(std::make_unique<Wt::WProgressBar>());
        progressBar->setRange(0, 1);
        // Update the progress bar periodically
        progress_bar_timer = this->addChild(std::make_unique<Wt::WTimer>());
        progress_bar_timer->setInterval(std::chrono::milliseconds(1000)); // Update every second

        Lauchbutton->clicked().connect([=] {
            Lauchbutton->setEnabled(false);
            delegate->willLaunchSimulator();
            current_time_stamp_string = get_time_stamp();
            std::string cmd_string = std::string(command_path).append(" -c ").
                    append(working_folder).append("/config_files/ -o ").
                    append(working_folder).append("/sim_results/ -t ").
                    append(current_time_stamp_string);
            progress_bar_timer->timeout().connect([=] {
                // Read the shared memory for the progress value
                int progress = readSharedMemoryProgress(std::string("/progress_").append(current_time_stamp_string));
                int total_num_task = readSharedMemoryProgress(std::string("/total_").append(current_time_stamp_string));
                int data_dump_ready = readSharedMemoryProgress(std::string("/datardy_").append(current_time_stamp_string));
                progressBar->setRange(0, total_num_task);
                progressBar->setValue(progress);
                if (data_dump_ready==1 && progress!=0 && total_num_task!=-1 && progress == total_num_task) {
                    // Re-enable the button
                    Lauchbutton->enable();
                    progress_bar_timer->stop();
                    Downloadbutton->enable();
                    Downloadbutton->setText(std::string("Download Results: \n").append(current_task_name).append(current_time_stamp_string));
                }
            });

            std::thread([=] {
                int result = std::system(cmd_string.c_str());
            }).detach();
            progress_bar_timer->start();
//            outputText->setText(std::string(result));
//            delegate->didLaunchSimulator();
        });

        Downloadbutton = buttonHLayout->addWidget(std::make_unique<Wt::WPushButton>("Download Results"));
        Downloadbutton->disable();
        Downloadbutton->clicked().connect(this, &MewLaunchPad::onDownloadClicked);
    }

    void onDownloadClicked();

    int readSharedMemoryProgress(std::string var_name) {
        int fd = shm_open(var_name.c_str(), O_RDONLY, 0);
        if (fd == -1) {
            std::cerr << "Error opening shared memory:" << var_name << std::endl;
            return 1;
        }
        // Size of the shared memory object
        const size_t sharedSize = sizeof(int);
        // Memory map the shared memory object
        void* ptr = mmap(0, sharedSize, PROT_READ, MAP_SHARED, fd, 0);
        if (ptr == MAP_FAILED) {
            std::cerr << "Error mapping shared memory:" << var_name << std::endl;
            return 1;
        }

        // Read from the shared memory
        int value;
        memcpy(&value, ptr, sizeof(value));
        std::cout << "Read from shared memory: " << value << std::endl;

        // Unmap and close the shared memory object
        munmap(ptr, sharedSize);
        close(fd);
        return value;
    }

    std::string executeCommand(const char* cmd) {
        std::thread thread{[cmd]() {
            std::system(cmd);
        }};
        thread.detach();
        return std::string("Launched!");
    }

    std::string get_time_stamp() {
        time_t rawtime;
        struct tm * timeinfo;
        char buffer[80];
        time (&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(buffer,sizeof(buffer),"%Y%m%d%H%M%S",timeinfo);
        std::string time_str(buffer);
        return time_str;
    };
};


#endif //MEWTWOMEGAEVO_MEWLAUNCHPAD_H
