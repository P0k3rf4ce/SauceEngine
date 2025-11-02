#define STB_IMAGE_IMPLEMENTATION

#include <QApplication>

#include <iostream>

#include "launcher/optionParser.hpp"
#include "launcher/launcherWindow.hpp"

int main(int argc, const char *argv[]) {
    const AppOptions ops(argc, argv);

    if (ops.help) {
        std::cout << "Usage: " << argv[0] << " <options> [scene_file]" << std::endl;
        std::cout << ops.getHelpMessage() << std::endl;
        return 1;
    }

    QApplication app(argc, const_cast<char**>(argv));
    LauncherWindow window(argc, argv);
    window.show();
    return app.exec();
}