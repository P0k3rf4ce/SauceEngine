#pragma once

#include <app/ui/ImGuiComponent.hpp>
#include <functional>
#include <imgui.h>
#include <string>

namespace sauce::ui {

    /**
 * TreeNode - Wraps ImGui::TreeNode() / ImGui::TreePop()
 * Creates a collapsible tree hierarchy element
 */
    class TreeNode : public ImGuiComponent {
      public:
        using ContentCallback = std::function<void()>;

        TreeNode(const std::string& name, const std::string& label,
                 ContentCallback content = nullptr, ImGuiTreeNodeFlags flags = 0);

        void render() override;

        void setLabel(const std::string& newLabel) {
            label = newLabel;
        }
        const std::string& getLabel() const {
            return label;
        }

        void setContent(ContentCallback newContent) {
            content = newContent;
        }
        void setFlags(ImGuiTreeNodeFlags newFlags) {
            flags = newFlags;
        }

        bool isNodeOpen() const {
            return nodeOpen;
        }

      private:
        std::string label;
        ContentCallback content;
        ImGuiTreeNodeFlags flags;
        bool nodeOpen = false;
    };

} // namespace sauce::ui
