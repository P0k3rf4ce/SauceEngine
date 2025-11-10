#ifndef LAUNCHER_WINDOW_HPP
#define LAUNCHER_WINDOW_HPP

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>

#include <unordered_map>
#include <memory>
#include <vector>
#include <string>

using OptionsMapValue = std::pair<std::unique_ptr<QLabel>, std::unique_ptr<QLineEdit>>;
using OptionsMap = std::unordered_map<std::string, OptionsMapValue>;

class LauncherWindow : public QWidget {
    Q_OBJECT

public:
    LauncherWindow(int argc, const char *argv[], QWidget *parent = nullptr);

private slots:
    void onLaunchClicked();

private:
    std::vector<std::string> options;
    OptionsMap optionsMap;
};

#endif