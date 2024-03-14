//
// Created by Rocky Su on 4/3/2024.
// GPT4 assisted
#include "MewLoginPortalApp.h"
#include <Wt/WApplication.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WLineEdit.h>
#include <Wt/WPushButton.h>
#include <Wt/WText.h>

LoginApplication::LoginApplication(const Wt::WEnvironment& env) : Wt::WApplication(env) {
    auto sqlite3 = std::make_unique<dbo::backend::Sqlite3>("userdb.sqlite");
    session.setConnection(std::move(sqlite3));

    session.mapClass<User>("user");
    try {
        session.createTables(); // This will attempt to create the tables if they don't exist
    } catch (const std::exception& e) {
        Wt::log("error") << "Database operation failed: " << e.what();
    }

    // Show login or registration form
}


void LoginApplication::registerUser(const std::string& username, const std::string& password) {
    dbo::Transaction transaction(session);
    std::unique_ptr<User> user(new User());
    user->username = username;
    user->password = password; // Remember, you should hash this!

    session.add(std::move(user));
    transaction.commit();
}

bool LoginApplication::checkLogin(const std::string& username, const std::string& password) {
    dbo::Transaction transaction(session);
    dbo::ptr<User> user = session.find<User>().where("username = ?").bind(username).where("password = ?").bind(password); // Remember to hash password in real scenarios

    return user ? true : false;
}
