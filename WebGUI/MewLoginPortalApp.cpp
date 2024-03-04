//
// Created by Rocky Su on 4/3/2024.
//
#include <Wt/WApplication.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WLineEdit.h>
#include <Wt/WPushButton.h>
#include <Wt/WText.h>

class MewLoginApp : public Wt::WApplication {
public:
    MewLoginApp(const Wt::WEnvironment& env) : Wt::WApplication(env) {
        setTitle("Login Portal");

        auto container = root()->addWidget(std::make_unique<Wt::WContainerWidget>());
        auto usernameEdit = container->addWidget(std::make_unique<Wt::WLineEdit>());
        usernameEdit->setPlaceholderText("Username");

        auto passwordEdit = container->addWidget(std::make_unique<Wt::WLineEdit>());
        passwordEdit->setPlaceholderText("Password");
        passwordEdit->setEchoMode(Wt::EchoMode::Password);

        auto loginButton = container->addWidget(std::make_unique<Wt::WPushButton>("Login"));
        loginButton->clicked().connect([=] {
            // Simple authentication check for demonstration
            if (usernameEdit->text() == "user" && passwordEdit->text() == "pass") {
                // Redirect to the main application upon successful login
                Wt::WApplication::instance()->redirect("http://localhost:8082/simconfig");
            } else {
                root()->addWidget(std::make_unique<Wt::WText>("Invalid username or password."));
            }
        });
    }
};