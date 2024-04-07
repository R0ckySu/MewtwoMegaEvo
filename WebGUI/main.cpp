#include <Wt/WServer.h>
//#include <Wt/WApplication.h>
#include "MewSimApp.h"
//#include "MewLoginPortalApp.h"
#include "MewUserInventoryApp.h"
#include "auth2/Auth2.h"

std::unique_ptr<Wt::WApplication> createUserInventoryApp(const Wt::WEnvironment& env) {
    return std::make_unique<MewUserInventoryApp>(env);
}

std::unique_ptr<Wt::WApplication> createSimConfigApp(const Wt::WEnvironment& env) {
    return std::make_unique<MewtwoSimConfigApp>(env);
}

std::unique_ptr<Wt::WApplication> createApplication(const Wt::WEnvironment& env) {
    return std::make_unique<AuthApplication>(env);
}

int main(int argc, char *argv[]) {
    try {
        Wt::WServer server(argc, argv, WTHTTP_CONFIGURATION);
        server.addEntryPoint(Wt::EntryPointType::Application, &createApplication, "/");
        Session::configureAuth();
        // Add a route for the login application
        server.addEntryPoint(Wt::EntryPointType::Application, &createSimConfigApp, "/simconfig");
        // Add a route for the main application
        server.addEntryPoint(Wt::EntryPointType::Application, &createUserInventoryApp, "/userInv");
        server.run();
    } catch (Wt::WServer::Exception& e) {
        std::cerr << e.what() << std::endl;
    } catch (std::exception& e) {
        std::cerr << "exception: " << e.what() << std::endl;
    }
}