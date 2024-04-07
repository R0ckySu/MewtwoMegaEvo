//
// Created by Rocky Su on 4/3/2024.
//
#ifndef MEWTWOMEGAEVO_MEWSIMAPP_H
#define MEWTWOMEGAEVO_MEWSIMAPP_H

#include <Wt/WApplication.h>
//#include <Wt/WEnvironment.h>
#include "MewLaunchPad.h"
#include "MewFilePannel.h"
#include "MewSimConfigPannel.h"
#include "MewHamiltonian.h"
#include "MewGate.h"

class MewtwoSimConfigApp : public Wt::WApplication, public TaskLaunchDelegate {
public:
    std::string userID;
    MewLaunchPad* LaunchPad;
    MewFilePannel* fileBrowser;
    MewSimConfigPannel* simConfigPannel;
    MewGateConfig* gateConfigTable;
    MewHamiltonianConfig* hamiltonianTable;
    MewtwoSimConfigApp(const Wt::WEnvironment& env);

    void willLaunchSimulator();
    void didLaunchSimulator();
};

#endif //MEWTWOMEGAEVO_MEWSIMAPP_H
