#include <editor/EditorApp.hpp>
#include <editor/OffscreenFramebuffer.hpp>
#include <editor/panels/ViewportPanel.hpp>

#include <cmath>
#include <filesystem>
#include <imgui.h>
#include <imgui_internal.h>

namespace sauce::editor {

    ViewportPanel::ViewportPanel(EditorApp& app) : EditorPanel("Viewport", app) {
    }

    void ViewportPanel::render() {
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin(title.c_str(), &isOpen, flags);
        ImGui::PopStyleVar();

        hovered = ImGui::IsWindowHovered();
        focused = ImGui::IsWindowFocused();

        ImVec2 size = ImGui::GetContentRegionAvail();

        // Detect viewport size changes (clamped to min 1x1)
        size.x = std::max(size.x, 1.0f);
        size.y = std::max(size.y, 1.0f);

        if (std::abs(size.x - viewportSize.x) > 0.5f || std::abs(size.y - viewportSize.y) > 0.5f) {
            viewportSize = size;
            sizeChanged = true;
        }

        // Display the offscreen framebuffer texture using DrawList directly
        // (bypasses ImGui::Image layout/clipping which can reject the draw after dock resize)
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        viewportScreenPos = cursorPos;
        auto* offscreenFB = app.getOffscreenFramebuffer();
        if (offscreenFB) {
            VkDescriptorSet ds = offscreenFB->getImGuiDescriptorSet();
            if (ds != VK_NULL_HANDLE) {
                ImGui::GetWindowDrawList()->AddImage(
                    reinterpret_cast<ImTextureID>(ds), cursorPos,
                    ImVec2(cursorPos.x + size.x, cursorPos.y + size.y));
            }
        }

        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 contentMin = ImGui::GetWindowContentRegionMin();

        // Viewport mode toggle button (top-right corner)
        // Drawn before InvisibleButton so it gets click priority in its rect
        {
            ViewportMode currentMode = app.getViewportMode();
            const char* vpModeStr = (currentMode == ViewportMode::Lit) ? "Lit" : "Unlit";
            float btnWidth = 60.0f;
            float btnHeight = 22.0f;
            float btnX = windowPos.x + contentMin.x + size.x - btnWidth - 10;
            float btnY = windowPos.y + contentMin.y + 5;

            ImGui::SetCursorScreenPos(ImVec2(btnX, btnY));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.15f, 0.18f, 0.85f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.25f, 0.32f, 0.90f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.35f, 0.35f, 0.45f, 0.95f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
            if (ImGui::Button(vpModeStr, ImVec2(btnWidth, btnHeight))) {
                if (currentMode == ViewportMode::Unlit) {
                    app.setViewportMode(ViewportMode::Lit);
                } else {
                    app.setViewportMode(ViewportMode::Unlit);
                }
            }
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(3);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Toggle viewport shading mode (Unlit / Lit)");
            }
        }

        // Make the viewport area a drop target for GLTF files
        ImGui::SetCursorScreenPos(cursorPos);
        ImGui::InvisibleButton("##ViewportDropTarget", size);
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_FILE")) {
                std::string path(static_cast<const char*>(payload->Data));
                std::string ext = std::filesystem::path(path).extension().string();
                if (ext == ".gltf" || ext == ".glb") {
                    app.importGLTFToScene(path);
                }
            }
            ImGui::EndDragDropTarget();
        }

        // Draw overlay info on top
        ImVec2 overlayPos = ImVec2(windowPos.x + contentMin.x + 10, windowPos.y + contentMin.y + 5);

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        auto& editorCamera = app.getEditorCamera();
        const char* modeStr = editorCamera.getMode() == EditorCamera::Mode::Orbit ? "Orbit" : "Fly";

        // Semi-transparent background for readability
        float lineHeight = ImGui::GetTextLineHeightWithSpacing();
        ImVec2 bgMin = ImVec2(overlayPos.x - 4, overlayPos.y - 2);
        ImVec2 bgMax = ImVec2(overlayPos.x + 420, overlayPos.y + lineHeight * 3 + 4);
        drawList->AddRectFilled(bgMin, bgMax, IM_COL32(0, 0, 0, 140), 4.0f);

        // Camera mode
        char buf[128];
        snprintf(buf, sizeof(buf), "Camera: %s", modeStr);
        drawList->AddText(overlayPos, IM_COL32(200, 200, 200, 255), buf);

        // FPS
        float dt = app.getDeltaTime();
        float fps = dt > 0.0001f ? 1.0f / dt : 0.0f;
        snprintf(buf, sizeof(buf), "FPS: %.0f (%.2f ms)", fps, dt * 1000.0f);
        drawList->AddText(ImVec2(overlayPos.x, overlayPos.y + lineHeight),
                          IM_COL32(200, 200, 200, 255), buf);

        // Controls hint
        drawList->AddText(
            ImVec2(overlayPos.x, overlayPos.y + lineHeight * 2), IM_COL32(140, 140, 140, 255),
            "LMB: Select | MMB: Pan | Scroll: Zoom | RMB: Fly | F: Focus | W/E/R: Gizmo");

        ImGui::End();
    }

} // namespace sauce::editor
