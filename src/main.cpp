#include <iostream>

#include <launcher/optionParser.hpp>
#include <launcher/launcherWindow.hpp>
#include <app/SauceEngineApp.hpp>

int main(int argc, const char *argv[]) {
  const AppOptions ops(argc, argv);

  if (ops.help) {
    std::cout << "Usage: " << argv[0] << " <options> [scene_file]" << std::endl;
    std::cout << ops.getHelpMessage() << std::endl;
    return 1;
  }

  if (ops.skip_launcher) {
    std::cout << "Skipping Launcher ..." << std::endl;
    SauceEngineApp mainApp;
    try {
      mainApp.run();
    } catch (std::exception& e) {
      std::cerr << e.what() << std::endl;
      return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
  }
  return startupLauncher(argc, argv);
}
