add_library(auth2
  Auth2.C
  AuthWidget.C
  RegistrationView.C
  model/Session.C
  model/User.C
  model/UserDetailsModel.C
        Auth2.h)

TARGET_LINK_LIBRARIES(auth2
        Wt::Wt
        Wt::HTTP
        Wt::DboSqlite3
        Wt::Dbo
        )

#INCLUDE_DIRECTORIES(${WT_SOURCE_DIR}/src)
