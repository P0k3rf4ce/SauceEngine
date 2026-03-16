#pragma once

#include <string>

namespace sauce::editor {

class EditorApp;

class EditorPanel {
public:
  EditorPanel(const std::string& title, EditorApp& app)
    : title(title), app(app) {}

  virtual ~EditorPanel() = default;

  virtual void render() = 0;

  const std::string& getTitle() const { return title; }
  bool& getOpenRef() { return isOpen; }
  bool getIsOpen() const { return isOpen; }
  void setOpen(bool open) { isOpen = open; }

protected:
  std::string title;
  EditorApp& app;
  bool isOpen = true;
};

} // namespace sauce::editor
