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

class TaskLaunchDelegate {
public:
    std::string task_name;
    virtual void willLaunchSimulator() = 0;
    virtual void didLaunchSimulator() = 0;
};

class MewLaunchPad: public Wt::WContainerWidget{
public:
    Wt::WTimer* progress_bar_timer;
    Wt::WPushButton* Lauchbutton;

    std::string most_recent_job_time_stamp;
    TaskLaunchDelegate* delegate;
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
        progress_bar_timer->timeout().connect([=] {
            // Read the shared memory for the progress value
            int progress = readSharedMemoryProgress(std::string("/mew_task_progress"));
            int total_num_task = readSharedMemoryProgress(std::string("/mew_total_task"));
            progressBar->setRange(0, total_num_task);
            progressBar->setValue(progress);
            if (progress!=0 && total_num_task!=0 && progress == total_num_task) {
                // Re-enable the button
                Lauchbutton->enable();
                progress_bar_timer->stop();
            }
        });

        Lauchbutton->clicked().connect([=] {
            Lauchbutton->setEnabled(false);
            delegate->willLaunchSimulator();
            std::string cmd_string = std::string("/Users/rockysu/CodeRepo/MewtwoMegaEvo.git/Playground/MewtwoMegaEvo -c /Users/rockysu/CodeRepo/MewtwoMegaEvo.git/Playground/config_files/ -o /Users/rockysu/CodeRepo/MewtwoMegaEvo.git/Playground/sim_results/");
            std::thread([=] {
                // Replace 'ls' with your long-running command
                int result = std::system(cmd_string.c_str());
            }).detach();
            //            auto result = executeCommand(cmd_string.c_str());
            progress_bar_timer->start();
//            outputText->setText(std::string(result));
//            delegate->didLaunchSimulator();
        });

        auto Downloadbutton = buttonHLayout->addWidget(std::make_unique<Wt::WPushButton>("Download Results"));
        Lauchbutton->clicked().connect([=] {
//            std::cout << std::string(readSharedMemoryTaskName()) << std::endl;
//            auto result = executeCommand("/Users/rockysu/CodeRepo/MewtwoMegaEvo.git/Playground/MewtwoMegaEvo -c /Users/rockysu/CodeRepo/MewtwoMegaEvo.git/Playground/config_files/ -o /Users/rockysu/CodeRepo/MewtwoMegaEvo.git/Playground/sim_results/");
//            outputText->setText(std::string(result));
        });
    }

    int readSharedMemoryProgress(std::string var_name) {
        int fd = shm_open(var_name.c_str(), O_RDONLY, 0);
        if (fd == -1) {
            std::cerr << "Error opening shared memory." << std::endl;
//            return -1;
        }
        // Size of the shared memory object
        const size_t sharedSize = sizeof(int);
        // Memory map the shared memory object
        void* ptr = mmap(0, sharedSize, PROT_READ, MAP_SHARED, fd, 0);
        if (ptr == MAP_FAILED) {
            std::cerr << "Error mapping shared memory." << std::endl;
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

//    const char* readSharedMemoryTaskName() {
//        int fd = shm_open("/mew_task_name", O_RDONLY, 0);
//        if (fd == -1) {
//            std::cerr << "Error opening shared memory." << std::endl;
////            return -1;
//        }
//        // Size of the shared memory object
//        const size_t sharedSize = sizeof(const char*);
//        // Memory map the shared memory object
//        void* ptr = mmap(0, sharedSize, PROT_READ, MAP_SHARED, fd, 0);
//        if (ptr == MAP_FAILED) {
//            std::cerr << "Error mapping shared memory." << std::endl;
//        }
//
//        // Read from the shared memory
//        const char * value;
//        memcpy(&value, ptr, sizeof(value));
//        std::cout << "Read from shared memory: " << value << std::endl;
//
//        // Unmap and close the shared memory object
//        munmap(ptr, sharedSize);
//        close(fd);
//        return value;
//    }

    std::string executeCommand(const char* cmd) {
        std::thread thread{[cmd]() {
            std::system(cmd);
        }};
        thread.detach();
        return std::string("Launched!");
    }
};


#endif //MEWTWOMEGAEVO_MEWLAUNCHPAD_H
