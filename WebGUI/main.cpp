#include <Wt/WApplication.h>
#include <Wt/WCheckBox.h>
#include <Wt/WHBoxLayout.h>
#include "MewHamiltonian.h"
#include "MewGate.h"
#include "MewSimConfigPannel.h"
#include "MewFilePannel.h"
#include "MewLaunchPad.h"
#include <iostream>

class MyApp : public Wt::WApplication {
public:
    MyApp(const Wt::WEnvironment& env) : Wt::WApplication(env) {
        useStyleSheet("https://fonts.googleapis.com/css?family=Roboto&display=swap");
        root()->setAttributeValue("style", "font-family: 'Roboto', sans-serif;");
//        root()->addWidget(std::make_unique<Wt::WText>("Mewtwo"));

        auto container_upper = root()->addWidget(std::make_unique<Wt::WContainerWidget>());
        container_upper->setOverflow(Wt::Overflow::Auto);
        auto hLayout_upper = container_upper->setLayout(std::make_unique<Wt::WHBoxLayout>());
        auto fileBrowser = hLayout_upper->addWidget(std::make_unique<MewFilePannel>("/Users/rockysu/CodeRepo/MewtwoMegaEvo.git/Playground/config_files/"));
        auto LaunchPad = hLayout_upper->addWidget(std::make_unique<MewLaunchPad>());

        auto container_lower = root()->addWidget(std::make_unique<Wt::WContainerWidget>());
        container_lower->setOverflow(Wt::Overflow::Auto);
        auto hLayout_lower = container_lower->setLayout(std::make_unique<Wt::WHBoxLayout>());

        auto pannel1 = std::make_unique<MewSimConfigPannel>();
        pannel1->setStyleClass("scrollable-table");
        pannel1->load_from_file("/Users/rockysu/CodeRepo/MewtwoMegaEvo.git/Playground/config_files/sim_config.json");
        hLayout_lower->addWidget(std::move(pannel1));

        auto table1 = std::make_unique<MewGateConfig>();
        table1->setStyleClass("scrollable-table");
        table1->load_from_file("/Users/rockysu/CodeRepo/MewtwoMegaEvo.git/Playground/config_files/gate_config.json");
        hLayout_lower->addWidget(std::move(table1));

        auto table2 = std::make_unique<MewHamiltonianConfig>();
        table2->setStyleClass("scrollable-table");
        table2->load_from_file("/Users/rockysu/CodeRepo/MewtwoMegaEvo.git/Playground/config_files/hamiltonian_config.json");
        hLayout_lower->addWidget(std::move(table2));
    }
};

int main(int argc, char **argv) {
    return Wt::WRun(argc, argv, [](const Wt::WEnvironment &env) {
        return std::make_unique<MyApp>(env);
    });
}
