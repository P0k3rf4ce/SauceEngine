#include <editor/EditorApp.hpp>
#include <iostream>

int main() {
  try {
    sauce::editor::EditorApp app;
    app.run();
  } catch (const std::exception& e) {
    std::cerr << "Fatal error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
