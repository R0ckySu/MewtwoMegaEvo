//
// Created by Rocky Su on 9/2/2024.
//

#ifndef MEWTWOMEGAEVO_MEWLAUNCHPAD_H
#define MEWTWOMEGAEVO_MEWLAUNCHPAD_H

#include <Wt/WContainerWidget.h>
#include <Wt/WTextArea.h>
#include <Wt/WPushButton.h>
#include <Wt/WVBoxLayout.h>
#include <Wt/WHBoxLayout.h>

class TaskLaunchDelegate {
public:
    virtual void willLaunchSimulator() = 0;
    virtual void didLaunchSimulator() = 0;
};

class MewLaunchPad: public Wt::WContainerWidget{
public:
    TaskLaunchDelegate* delegate;
    MewLaunchPad() : Wt::WContainerWidget() {
        auto vLayout = this->setLayout(std::make_unique<Wt::WVBoxLayout>());
//        auto container_up = addWidget(std::make_unique<Wt::WContainerWidget>());
        auto outputText = vLayout->addWidget(std::make_unique<Wt::WTextArea>());
        outputText->setReadOnly(true); // Make it read-only if editing is not required
//        outputText->resize(500, 200);

        auto buttonContainer = vLayout->addWidget(std::make_unique<Wt::WContainerWidget>());
        auto buttonHLayout = buttonContainer->setLayout(std::make_unique<Wt::WHBoxLayout>());
        auto Lauchbutton = buttonHLayout->addWidget(std::make_unique<Wt::WPushButton>("Launch Simulator!"));
        Lauchbutton->clicked().connect([=] {
            delegate->willLaunchSimulator();
            auto result = executeCommand("/Users/rockysu/CodeRepo/MewtwoMegaEvo.git/Playground/MewtwoMegaEvo -c /Users/rockysu/CodeRepo/MewtwoMegaEvo.git/Playground/config_files/ -o /Users/rockysu/CodeRepo/MewtwoMegaEvo.git/Playground/sim_results/");
            outputText->setText(std::string(result));
//            delegate->didLaunchSimulator();
        });
        auto Downloadbutton = buttonHLayout->addWidget(std::make_unique<Wt::WPushButton>("Download Results"));
        Lauchbutton->clicked().connect([=] {
//            auto result = executeCommand("/Users/rockysu/CodeRepo/MewtwoMegaEvo.git/Playground/MewtwoMegaEvo -c /Users/rockysu/CodeRepo/MewtwoMegaEvo.git/Playground/config_files/ -o /Users/rockysu/CodeRepo/MewtwoMegaEvo.git/Playground/sim_results/");
//            outputText->setText(std::string(result));
        });
    }

    std::string executeCommand(const char* cmd) {
        std::array<char, 1024> buffer;
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
        if (!pipe) {
            throw std::runtime_error("popen() failed!");
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        return result;
    }
};


#endif //MEWTWOMEGAEVO_MEWLAUNCHPAD_H
