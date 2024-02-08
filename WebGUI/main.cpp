#include <Wt/WApplication.h>
#include <Wt/WTextArea.h>
#include <Wt/WCheckBox.h>
#include <Wt/WHBoxLayout.h>
#include "MewHamiltonian.h"
#include "MewGate.h"
#include "MewSimConfigPannel.h"
#include <iostream>

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


class MyApp : public Wt::WApplication {
public:
    MyApp(const Wt::WEnvironment& env) : Wt::WApplication(env) {
        useStyleSheet("https://fonts.googleapis.com/css?family=Roboto&display=swap");
        root()->setAttributeValue("style", "font-family: 'Roboto', sans-serif;");
        root()->addWidget(std::make_unique<Wt::WText>("Mewtwo"));

        auto container_up = root()->addWidget(std::make_unique<Wt::WContainerWidget>());
        auto button = container_up->addWidget(std::make_unique<Wt::WPushButton>("Run Command"));
        auto outputText = container_up->addWidget(std::make_unique<Wt::WTextArea>());
        outputText->setReadOnly(true); // Make it read-only if editing is not required
        outputText->resize(300, 200);

        button->clicked().connect([=] {
            auto result = executeCommand("/Users/rockysu/CodeRepo/MewtwoMegaEvo.git/Playground/MewtwoMegaEvo -c /Users/rockysu/CodeRepo/MewtwoMegaEvo.git/Playground/config_files/ -o /Users/rockysu/CodeRepo/MewtwoMegaEvo.git/Playground/sim_results/");
            outputText->setText(std::string(result));
        });

        auto container = root()->addWidget(std::make_unique<Wt::WContainerWidget>());
        container->setOverflow(Wt::Overflow::Auto);
        auto hLayout = container->setLayout(std::make_unique<Wt::WHBoxLayout>());

        auto pannel1 = std::make_unique<MewSimConfigPannel>();
        pannel1->setStyleClass("scrollable-table");
        pannel1->load_from_file("/Users/rockysu/CodeRepo/MewtwoMegaEvo.git/Playground/config_files/sim_config.json");
        hLayout->addWidget(std::move(pannel1));

        auto table1 = std::make_unique<MewGateConfig>();
        table1->setStyleClass("scrollable-table");
        table1->load_from_file("/Users/rockysu/CodeRepo/MewtwoMegaEvo.git/Playground/config_files/gate_config.json");
        hLayout->addWidget(std::move(table1));

        auto table2 = std::make_unique<MewHamiltonianConfig>();
        table2->setStyleClass("scrollable-table");
        table2->load_from_file("/Users/rockysu/CodeRepo/MewtwoMegaEvo.git/Playground/config_files/hamiltonian_config.json");
        hLayout->addWidget(std::move(table2));
    }
};

int main(int argc, char **argv) {
    return Wt::WRun(argc, argv, [](const Wt::WEnvironment &env) {
        return std::make_unique<MyApp>(env);
    });
}
