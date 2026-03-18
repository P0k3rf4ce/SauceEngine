#include <editor/EditorApp.hpp>
#include <iostream>

int main(int argc, char** argv) {
    try {
        sauce::editor::EditorApp app;

        // Pass scene file path if provided as first argument
        std::string sceneFile;
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg.ends_with(".gltf") || arg.ends_with(".glb")) {
                sceneFile = arg;
                break;
            }
        }
        if (!sceneFile.empty()) {
            app.setInitialSceneFile(sceneFile);
        }

        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
