//
// Created by Rocky Su on 9/3/2024.
//

#ifndef MEWTWOMEGAEVO_MEWLOGINPORTALAPP_H
#define MEWTWOMEGAEVO_MEWLOGINPORTALAPP_H

#include <Wt/Dbo/Dbo.h>
#include <Wt/Dbo/WtSqlTraits.h>
#include <Wt/WApplication.h>
#include <Wt/Dbo/Dbo.h>
#include <Wt/Dbo/backend/Sqlite3.h>

namespace dbo = Wt::Dbo;

class User {
public:
    std::string username;
    std::string password;

    template<class Action>
    void persist(Action& a) {
        dbo::field(a, username, "username");
        dbo::field(a, password, "password");
    }
};

namespace dbo = Wt::Dbo;

class LoginApplication : public Wt::WApplication {
public:
    LoginApplication(const Wt::WEnvironment& env);

private:
    dbo::Session session;
    std::unique_ptr<dbo::backend::Sqlite3> sqlite3; // Use a unique_ptr for the SQLite3 connection

//    void initializeDatabase();
//    void showLoginForm();
//    void showRegistrationForm();
    void registerUser(const std::string& username, const std::string& password);
    bool checkLogin(const std::string& username, const std::string& password);
};


#endif //MEWTWOMEGAEVO_MEWLOGINPORTALAPP_H
