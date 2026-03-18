#include <app/ui/components/TreeNode.hpp>

namespace sauce::ui {

    TreeNode::TreeNode(const std::string& name, const std::string& label, ContentCallback content,
                       ImGuiTreeNodeFlags flags)
        : ImGuiComponent(name), label(label), content(content), flags(flags) {
    }

    void TreeNode::render() {
        if (!enabled)
            return;

        nodeOpen = ImGui::TreeNodeEx(label.c_str(), flags);
        if (nodeOpen) {
            if (content) {
                content();
            }
            ImGui::TreePop();
        }
    }

} // namespace sauce::ui
