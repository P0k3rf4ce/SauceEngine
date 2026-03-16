#pragma once

#include <app/ui/ImGuiComponent.hpp>
#include <app/Settings.hpp>
#include <imgui.h>
#include <string>
#include <algorithm>

#ifdef _WIN32
#  include <windows.h>
#  include <shellapi.h>
#elif defined(__APPLE__) || defined(__linux__)
#  include <unistd.h>
#endif

namespace sauce::ui {

class SettingsWindow : public ImGuiComponent {
public:
  explicit SettingsWindow(sauce::SettingsManager& settingsManager)
    : ImGuiComponent("SettingsWindow"), settings(settingsManager)
  {
    auto& s = settings.get();
    const std::size_t copyLen = std::min(s.workingDirectory.size(), sizeof(workingDirBuf) - 1);
    std::copy_n(s.workingDirectory.data(), copyLen, workingDirBuf);
    workingDirBuf[copyLen] = '\0';
  }

  void render() override {
    if (!enabled) return;

    ImGui::SetNextWindowSize(ImVec2(420, 0), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Settings", &enabled)) {
      ImGui::End();
      return;
    }

    auto& s = settings.get();
    bool changed = false;

    if (ImGui::CollapsingHeader("Display", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::SliderFloat("UI Scale", &s.imguiScale, 0.25f, 4.0f, "%.2fx");
      changed |= ImGui::IsItemDeactivatedAfterEdit();

      if (ImGui::Checkbox("V-Sync", &s.vsync)) {
        changed = true;
      }
      ImGui::SameLine();
      ImGui::TextDisabled("(requires restart)");
    }

    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::SliderFloat("Mouse Sensitivity", &s.mouseSensitivity, 0.01f, 1.0f, "%.2f");
      changed |= ImGui::IsItemDeactivatedAfterEdit();
      ImGui::SliderFloat("Movement Speed", &s.cameraSpeed, 0.5f, 20.0f, "%.1f");
      changed |= ImGui::IsItemDeactivatedAfterEdit();
      ImGui::SliderFloat("Field of View", &s.fieldOfView, 30.0f, 120.0f, "%.0f deg");
      changed |= ImGui::IsItemDeactivatedAfterEdit();
    }

    if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen)) {
      if (ImGui::InputText("Working Directory", workingDirBuf, sizeof(workingDirBuf),
                           ImGuiInputTextFlags_EnterReturnsTrue)) {
        s.workingDirectory = workingDirBuf;
        changed = true;
      }

      changed |= ImGui::Checkbox("Show Debug Stats", &s.showDebugStats);
    }

    if (ImGui::CollapsingHeader("Diagnostics", ImGuiTreeNodeFlags_DefaultOpen)) {
      changed |= ImGui::Checkbox("Palantir Mode (verbose logging)", &s.palantirMode);
      ImGui::SameLine();
      ImGui::TextDisabled("(?)");
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Enables detailed diagnostic logging to sauceengine.log");
      }
    }

    if (changed) {
      settings.markDirtyAndSave();
    }

    ImGui::Separator();
    ImGui::Spacing();
    if (ImGui::SmallButton("store.palantir.com")) {
      openStoreUrl();
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }

    ImGui::End();
  }

private:
  static void openStoreUrl() {
#ifdef _WIN32
    ShellExecuteA(nullptr, "open", "https://store.palantir.com", nullptr, nullptr, SW_SHOWNORMAL);
#elif defined(__APPLE__) || defined(__linux__)
    pid_t pid = fork();
    if (pid == 0) {
#  ifdef __APPLE__
      execlp("open", "open", "https://store.palantir.com", static_cast<char*>(nullptr));
#  else
      execlp("xdg-open", "xdg-open", "https://store.palantir.com", static_cast<char*>(nullptr));
#  endif
      _exit(127);
    }
#endif
  }

  sauce::SettingsManager& settings;
  char workingDirBuf[512] = {};
};

} // namespace sauce::ui
