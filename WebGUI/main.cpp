#include <Wt/WServer.h>
//#include <Wt/WApplication.h>
#include "MewSimApp.h"
#include "MewLoginPortalApp.cpp"

std::unique_ptr<Wt::WApplication> createSimConfigApp(const Wt::WEnvironment& env) {
    /*
    Your LoginApp class should be defined to return an instance of Wt::WApplication.
    */
    return std::make_unique<MewtwoSimConfigApp>(env);
}

std::unique_ptr<Wt::WApplication> createMewLoginApp(const Wt::WEnvironment& env) {
    /*
    Your LoginApp class should be defined to return an instance of Wt::WApplication.
    */
    return std::make_unique<MewLoginApp>(env);
}

int main(int argc, char *argv[]) {
    try {
        Wt::WServer server(argc, argv, WTHTTP_CONFIGURATION);
        server.addEntryPoint(Wt::EntryPointType::Application, &createMewLoginApp, "/");

        // Add a route for the login application
        server.addEntryPoint(Wt::EntryPointType::Application, &createSimConfigApp, "/simconfig");

        // Add a route for the main application

        server.run();

    } catch (Wt::WServer::Exception& e) {
        std::cerr << e.what() << std::endl;
    } catch (std::exception& e) {
        std::cerr << "exception: " << e.what() << std::endl;
    }
}