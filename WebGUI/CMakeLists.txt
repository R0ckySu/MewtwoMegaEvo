CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
Project(WebGUI)
set(CMAKE_CXX_STANDARD 17)

FIND_PACKAGE(Wt REQUIRED Wt HTTP)
ADD_DEFINITIONS(-DHPDF_DLL)

ADD_EXECUTABLE(WebGUI.wt
        main.cpp
        MewSimConfigPannel.cpp MewSimConfigPannel.h
        MewHamiltonian.h MewHamiltonian.cpp
        MewGate.h MewGate.cpp
        MewComponents.cpp MewComponents.h
        MewMatrixInputPannel.h
        MewFilePannel.cpp MewFilePannel.h
        MewLaunchPad.cpp MewLaunchPad.h
        MewSimApp.cpp MewSimApp.h
        MewUserInventoryApp.h MewUserInventoryApp.cpp
#        MewLoginPortalApp.h MewLoginPortalApp.cpp
        )

add_subdirectory(auth2)

TARGET_LINK_LIBRARIES(WebGUI.wt
        Wt::Wt
        Wt::HTTP
        auth2
        )