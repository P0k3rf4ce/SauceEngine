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

  SauceEngineApp mainApp;

  // Define a lambda for adding custom UI components
  auto customUIBuilder = [](sauce::ui::ImGuiComponentManager& manager) {
    manager.addComponent(std::make_unique<sauce::ui::ProgressBar>("ProgressBar", 0.5f));
    manager.addComponent(std::make_unique<sauce::ui::PlotLines>("PlotLines", std::vector<float>{0.1f, 0.9f, 0.2f, 0.8f, 0.3f, 0.7f, 0.4f, 0.6f, 0.5f}));
    manager.addComponent(std::make_unique<sauce::ui::PlotHistogram>("PlotHistogram", std::vector<float>{0.1f, 0.9f, 0.2f, 0.8f, 0.3f, 0.7f, 0.4f, 0.6f, 0.5f}));
    manager.addComponent(std::make_unique<sauce::ui::Button>("ButtonWithTooltip", "Button with Tooltip"));
    manager.addComponent(std::make_unique<sauce::ui::Tooltip>("Tooltip", "This is a tooltip"));
    manager.addComponent(std::make_unique<sauce::ui::Button>("ButtonWithCustomTooltip", "Button with Custom Tooltip"));
    manager.addComponent(std::make_unique<sauce::ui::CustomTooltip>("CustomTooltip", []() {
      ImGui::Text("This is a custom tooltip.");
      ImGui::Button("A button inside a tooltip");
    }));
  };

  // Register the custom UI builder
  mainApp.setCustomUIBuilder(customUIBuilder);

  try {
    mainApp.run();
  } catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}