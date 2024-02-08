//
// Created by Rocky Su on 27/1/2024.
//

#ifndef MYPROJECT_MEWMATRIXINPUTPANNEL_H
#define MYPROJECT_MEWMATRIXINPUTPANNEL_H

//#include <Wt/WApplication.h>
//#include <Wt/WDialog.h>
//#include <Wt/WPushButton.h>
//#include <Wt/WTable.h>
//#include <Wt/WLineEdit.h>
//
//class MyApp : public Wt::WApplication {
//public:
//    MyApp(const Wt::WEnvironment& env) : Wt::WApplication(env) {
//        auto container = root()->addWidget(std::make_unique<Wt::WContainerWidget>());
//
//        // Button to trigger the matrix input dialog
//        auto showDialogButton = container->addWidget(std::make_unique<Wt::WPushButton>("Enter Matrix"));
//        showDialogButton->clicked().connect([=] {
//            showMatrixInputDialog();
//        });
//    }
//
//    void showMatrixInputDialog() {
//        auto dialog = root()->addChild(std::make_unique<Wt::WDialog>("Matrix Input"));
//
//        // Matrix input (example: 3x3 matrix)
//        auto table = dialog->contents()->addWidget(std::make_unique<Wt::WTable>());
//        const int matrixSize = 3; // Example size, adjust as needed
//        for (int i = 0; i < matrixSize; ++i) {
//            for (int j = 0; j < matrixSize; ++j) {
//                auto edit = std::make_unique<Wt::WLineEdit>();
//                table->elementAt(i, j)->addWidget(std::move(edit));
//            }
//        }
//
//        // Submit button
//        auto submitButton = dialog->footer()->addWidget(std::make_unique<Wt::WPushButton>("Submit"));
//        submitButton->clicked().connect([=] {
//            // Logic to handle the matrix input
//            dialog->accept();
//        });
//
//        // Cancel button
//        auto cancelButton = dialog->footer()->addWidget(std::make_unique<Wt::WPushButton>("Cancel"));
//        cancelButton->clicked().connect([=] {
//            dialog->reject();
//        });
//
//        dialog->finished().connect([=] (Wt::DialogCode code) {
//            root()->removeChild(dialog);
//        });
//
//        dialog->show();
//    }
//};
//
//int main(int argc, char** argv) {
//    return Wt::WRun(argc, argv, [](const Wt::WEnvironment& env) {
//        return std::make_unique<MyApp>(env);
//    });
//}

#endif //MYPROJECT_MEWMATRIXINPUTPANNEL_H
