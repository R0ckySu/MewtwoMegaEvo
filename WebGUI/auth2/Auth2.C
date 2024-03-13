/*
 * Copyright (C) 2012 Emweb bv, Herent, Belgium.
 *
 * See the LICENSE file for terms of use.
 */
#include "Auth2.h"
#include <Wt/WApplication.h>
#include <Wt/WServer.h>
#include <Wt/Auth/PasswordService.h>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <memory>
#include <utility>
#include "model/Session.h"
#include "AuthWidget.h"

AuthApplication::AuthApplication(const Wt::WEnvironment& env): Wt::WApplication(env),
      session_(appRoot() + "auth.db")
{
    session_.login().changed().connect(this, &AuthApplication::authEvent);

    useStyleSheet("css/style.css");
    messageResourceBundle().use("strings");
    messageResourceBundle().use("templates");

    auto authWidget = std::make_unique<AuthWidget>(session_);

    authWidget->model()->addPasswordAuth(&Session::passwordAuth());
    authWidget->model()->addOAuth(Session::oAuth());
    authWidget->setRegistrationEnabled(true);

    authWidget->processEnvironment();

    root()->addWidget(std::move(authWidget));
}

void AuthApplication::authEvent() {
    if (session_.login().loggedIn()) {
      log("notice") << "User " << session_.login().user().id()
                        << " logged in.";
      Dbo::Transaction t(session_);
      dbo::ptr<User> user = session_.user();
      log("notice") << "(Favourite pet: " << user->favouritePet << ")";
    } else
      log("notice") << "User logged out.";
}


//int main(int argc, char **argv)
//{
//  try {
//    WServer server(argc, argv, WTHTTP_CONFIGURATION);
//
//    server.addEntryPoint(EntryPointType::Application, createApplication);
//
//    Session::configureAuth();
//
//    server.run();
//  } catch (WServer::Exception& e) {
//    std::cerr << e.what() << std::endl;
//  } catch (std::exception &e) {
//    std::cerr << "exception: " << e.what() << std::endl;
//  }
//}
