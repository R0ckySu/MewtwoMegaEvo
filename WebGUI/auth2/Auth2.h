//
// Created by Rocky Su on 12/3/2024.
//

#ifndef MEWTWOMEGAEVO_AUTH2_H
#define MEWTWOMEGAEVO_AUTH2_H

#include <Wt/WApplication.h>
#include <Wt/WEnvironment.h>
#include "model/Session.h"

class AuthApplication : public Wt::WApplication {
public:
    AuthApplication(const Wt::WEnvironment& env);
    void authEvent();

private:
    Session session_;
};

#endif //MEWTWOMEGAEVO_AUTH2_H
