#include "launcher/launcherWindow.hpp"
#include "launcher/optionParser.hpp"
#include "launcher/engineMainloop.hpp"

#include <QGridLayout>
#include <QIntValidator>
#include <QLineEdit>
#include <QToolButton>

#include <algorithm>

void initOptionsMap(std::vector<std::string> &options, OptionsMap &optionsMap, const AppOptions &defaultOptions) {
    options.push_back("scr_width");
    optionsMap["scr_width"] = std::make_pair(
        std::make_unique<QLabel>("Screen Width"), 
        std::make_unique<QLineEdit>(std::to_string(defaultOptions.scr_width).c_str())
    );
    optionsMap["scr_width"].second->setValidator(
        new QIntValidator(100, 100000, optionsMap["scr_width"].second.get())
    );
    
    options.push_back("scr_height");
    optionsMap["scr_height"] = std::make_pair(
        std::make_unique<QLabel>("Screen Height"), 
        std::make_unique<QLineEdit>(std::to_string(defaultOptions.scr_height).c_str())
    );
    optionsMap["scr_height"].second->setValidator(
        new QIntValidator(100, 100000, optionsMap["scr_height"].second.get())
    );

    options.push_back("tickrate");
    optionsMap["tickrate"] = std::make_pair(
        std::make_unique<QLabel>("Tickrate"), 
        std::make_unique<QLineEdit>(std::to_string(defaultOptions.tickrate).c_str())
    );
    optionsMap["tickrate"].second->setValidator(
        new QDoubleValidator(1.0, 1000.0, 2, optionsMap["scr_height"].second.get())
    );
    
    options.push_back("scene_file");
    optionsMap["scene_file"] = std::make_pair(
        std::make_unique<QLabel>("Scene File"), 
        std::make_unique<QLineEdit>(defaultOptions.scene_file.c_str())
    );
}

LauncherWindow::LauncherWindow(int argc, const char *argv[], QWidget *parent)
    : QWidget(parent)
{
    const AppOptions defaultOptions(argc, argv);

    QGridLayout *mainLayout = new QGridLayout;

    initOptionsMap(options, optionsMap, defaultOptions);

    for (int i = 0; i < options.size(); ++i) {
        auto key = options[i];
        mainLayout->addWidget(optionsMap[key].first.get(), i, 0);
        mainLayout->addWidget(optionsMap[key].second.get(), i, 1);
    }

    QToolButton *launchButton = new QToolButton();
    launchButton->setText("Launch");
    connect(launchButton, &QToolButton::clicked, this, &LauncherWindow::onLaunchClicked);

    mainLayout->addWidget(launchButton, options.size(), 1, Qt::AlignRight);

    setLayout(mainLayout);

    setWindowTitle(tr("SauceEngine Launcher"));
    setMinimumSize(400, 200);
    resize(500, 250);
}

void LauncherWindow::onLaunchClicked() {
    close();

    AppOptions ops = AppOptions();

    ops.scr_width = optionsMap["scr_width"].second->text().toUInt();
    ops.scr_height = optionsMap["scr_height"].second->text().toUInt();
    ops.tickrate = optionsMap["tickrate"].second->text().toDouble();
    ops.scene_file = optionsMap["scene_file"].second->text().toStdString();

    engine_mainloop(ops);
}