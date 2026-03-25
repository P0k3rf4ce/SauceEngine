#include <iostream>
#include <functional>
#include <vector>

#include <launcher/optionParser.hpp>
#include <app/SauceEngineApp.hpp>
#include <app/ui/ImGuiComponentManager.hpp>
#include <app/ui/components/Button.hpp>
#include <app/ui/components/CustomTooltip.hpp>
#include <app/ui/components/PlotHistogram.hpp>
#include <app/ui/components/PlotLines.hpp>
#include <app/ui/components/ProgressBar.hpp>
#include <app/ui/components/Tooltip.hpp>

int main(int argc, const char *argv[]) {
  const AppOptions ops(argc, argv);

  if (ops.help) {
    std::cout << "Usage: " << argv[0] << " <options> [scene_file]" << std::endl;
    std::cout << ops.getHelpMessage() << std::endl;
    return 1;
  }

  sauce::SauceEngineApp mainApp;
  try {
    if (!ops.scene_file.empty()) {
      mainApp.setSceneFile(ops.scene_file);
    }
    mainApp.setCameraCollisionEnabled(ops.camera_collide);
    if (!ops.ibl_file.empty()) {
      mainApp.setIBLFile(ops.ibl_file);
    }
    mainApp.run(ops.scr_width, ops.scr_height);
  } catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
