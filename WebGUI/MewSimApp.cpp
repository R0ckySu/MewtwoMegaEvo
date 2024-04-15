//
// Created by Rocky Su on 4/3/2024.
//

#include "MewSimApp.h"
#include <Wt/WCheckBox.h>
#include <Wt/WEnvironment.h>
#include <Wt/WHBoxLayout.h>


#include <iostream>

#define basePath "."

MewtwoSimConfigApp::MewtwoSimConfigApp(const Wt::WEnvironment& env) : Wt::WApplication(env) {
    userID = env.getParameter("uid") ? *env.getParameter("uid") : "defaultUserId";

    useStyleSheet("https://fonts.googleapis.com/css?family=Roboto&display=swap");
    root()->setAttributeValue("style", "font-family: 'Roboto', sans-serif;");
//        root()->addWidget(std::make_unique<Wt::WText>("Mewtwo"));

    auto container_upper = root()->addWidget(std::make_unique<Wt::WContainerWidget>());
    container_upper->setOverflow(Wt::Overflow::Auto);
    auto hLayout_upper = container_upper->setLayout(std::make_unique<Wt::WHBoxLayout>());
    fileBrowser = hLayout_upper->addWidget(std::make_unique<MewFilePannel>(std::string(basePath).append("/").append(userID).append("/config_files")));
    LaunchPad = hLayout_upper->addWidget(std::make_unique<MewLaunchPad>());
    LaunchPad->working_folder = std::string(basePath).append("/").append(userID);
    LaunchPad->delegate = this;

    auto container_lower = root()->addWidget(std::make_unique<Wt::WContainerWidget>());
    container_lower->setOverflow(Wt::Overflow::Auto);
    auto hLayout_lower = container_lower->setLayout(std::make_unique<Wt::WHBoxLayout>());

    simConfigPannel = hLayout_lower->addWidget(std::make_unique<MewSimConfigPannel>());
    simConfigPannel->setStyleClass("scrollable-table");
    simConfigPannel->load_from_file(std::string(basePath).append("/").append(userID).append("/config_files/sim_config.json"));

    gateConfigTable = hLayout_lower->addWidget(std::make_unique<MewGateConfig>());
    gateConfigTable->setStyleClass("scrollable-table");
    gateConfigTable->load_from_file(std::string(basePath).append("/").append(userID).append("/config_files/gate_config.json"));

    hamiltonianTable = hLayout_lower->addWidget(std::make_unique<MewHamiltonianConfig>());
    hamiltonianTable->setStyleClass("scrollable-table");
    hamiltonianTable->load_from_file(std::string(basePath).append("/").append(userID).append("/config_files/hamiltonian_config.json"));
};

void MewtwoSimConfigApp::willLaunchSimulator() {
    simConfigPannel->dump_config();
    gateConfigTable->dump_config();
    hamiltonianTable->dump_config();
    LaunchPad->current_task_name = simConfigPannel->task_name->get_data().second;
};

void MewtwoSimConfigApp::didLaunchSimulator() {
};