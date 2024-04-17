//
// Created by Rocky Su on 9/2/2024.
//

#include "MewLaunchPad.h"
#include <Wt/WFileResource.h>
#include <Wt/WAnchor.h>

void MewLaunchPad::onDownloadClicked() {
    std::string folderPath = std::string(working_folder).append("/sim_results/").append(current_task_name).append(current_time_stamp_string);
    std::string zipFileRelativePath = std::string(current_task_name).append(current_time_stamp_string);

    std::string command = "cd " + std::string(working_folder) + "/sim_results/ ;" +  "zip -r " + std::string(zipFileRelativePath).append(".zip") + " " + zipFileRelativePath;
    std::system(command.c_str());

    //wait(reinterpret_cast<int *>(100));

    // Proceed to serve the file for download.
    zipFileResource = std::make_shared<Wt::WFileResource>("application/zip", std::string(folderPath).append(".zip"));
    this->addWidget(std::make_unique<Wt::WAnchor>(zipFileResource, "Download"));
    Wt::WApplication::instance()->redirect(zipFileResource->url());
}