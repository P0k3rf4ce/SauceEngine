#pragma once

#include <imgui.h>
#include <imgui_stdlib.h>

#include <app/ui/ImGuiComponent.hpp>
#include <functional>
#include <optional>

// Make these helpers private to the file
namespace sauce::ui {

    namespace {
        template <typename T> using OnChange = std::function<void(T&)>;

        template <typename T>
        concept InputOneType =
            (std::is_same_v<T, int> || std::is_same_v<T, float> || std::is_same_v<T, double>);

        template <typename T, int N>
        concept InputManyType =
            (std::is_same_v<T, int> || std::is_same_v<T, float>) && 1 < N && N <= 4;
    } // namespace

    template <typename Derived, typename T> class InputBase : public ImGuiComponent {
      private:
        OnChange<T> func = [](auto arg) {};

      protected:
        const char* label = " ";
        T data;
        T* ref;
        bool check();

        InputBase(const char* name, const char* label, T data, T* ref, OnChange<T> func)
            : ImGuiComponent(name), label(label), data(data), ref(ref), func(func) {
        }

      public:
        T& get_data() {
            return this->ref ? *this->ref : this->data;
        }

        void render() override {
            if (!this->enabled)
                return;
            if (static_cast<Derived*>(this)->check())
                func(this->get_data());
        }

        template <typename T = Derived>
        static std::unique_ptr<Derived> make(const char* name, typename T::Args args) {
            return std::make_unique<Derived>(Derived(name, args));
        }
    };

    template <typename T>
        requires InputOneType<T>
    class InputOne : public InputBase<InputOne<T>, T> {
      public:
        struct Args {
            const char* label = " ";
            T init = (T)0.0;
            T step = (T)1.0;
            T step_fast = (T)1.0;
            const char* format = "%.3f";
            ImGuiInputTextFlags flags = 0;
            T* ref = nullptr;
            OnChange<T> func = [](auto arg) {};
        };

        InputOne(const char* name, Args args)
            : InputBase<InputOne<T>, T>(name, args.label, args.init, args.ref, args.func),
              step(args.step), step_fast(args.step_fast), flags(args.flags) {
        }

        bool check() {
            if constexpr (std::is_same_v<T, int>)
                return ImGui::InputInt(this->label, &this->get_data(), this->step, this->step_fast,
                                       this->flags);
            else if constexpr (std::is_same_v<T, float>)
                return ImGui::InputFloat(this->label, &this->get_data(), this->step,
                                         this->step_fast, this->format, this->flags);
            else if constexpr (std::is_same_v<T, double>)
                return ImGui::InputDouble(this->label, &this->get_data(), this->step,
                                          this->step_fast, this->format, this->flags);
        }

      private:
        T step;
        T step_fast;
        ImGuiInputTextFlags flags;
    };

    template <typename T, int N>
        requires InputManyType<T, N>
    class InputMany : public InputBase<InputMany<T, N>, std::array<T, N>> {
      public:
        struct Args {
            const char* label = " ";
            std::array<T, N> init = {0};
            ImGuiInputTextFlags flags = 0;
            std::array<T, N>* ref = nullptr;
            OnChange<std::array<T, N>> func = [](auto arg) {};
        };

        InputMany(const char* name, Args args = {})
            : InputBase<InputMany<T, N>, std::array<T, N>>(name, args.label, args.init, args.ref,
                                                           args.func),
              flags(args.flags) {
        }

        bool check() {
            if constexpr (std::is_same_v<T, int>) {
                if constexpr (N == 2)
                    return ImGui::InputInt2(this->label, this->get_data().data(), this->flags);
                if constexpr (N == 3)
                    return ImGui::InputInt3(this->label, this->get_data().data(), this->flags);
                if constexpr (N == 4)
                    return ImGui::InputInt4(this->label, this->get_data().data(), this->flags);
            } else if constexpr (std::is_same_v<T, float>) {
                if constexpr (N == 2)
                    return ImGui::InputFloat2(this->label, this->get_data().data(), "%.3f",
                                              this->flags);
                if constexpr (N == 3)
                    return ImGui::InputFloat3(this->label, this->get_data().data(), "%.3f",
                                              this->flags);
                if constexpr (N == 4)
                    return ImGui::InputFloat4(this->label, this->get_data().data(), "%.3f",
                                              this->flags);
            }
        }

      private:
        ImGuiInputTextFlags flags;
        std::array<T, N>* ref;
    };

    class InputTxt : public InputBase<InputTxt, std::string> {
      public:
        struct Args {
            const char* label = " ";
            std::string init = "";
            std::optional<ImVec2> shape = std::nullopt;
            ImGuiInputTextFlags flags = 0;
            ImGuiInputTextCallback callback = nullptr;
            void* user_data = nullptr;
            OnChange<std::string> func = [](auto arg) {};
        };
        InputTxt(const char* name, Args args = {})
            : InputBase<InputTxt, std::string>(name, args.label, args.init, nullptr, args.func),
              shape(args.shape), flags(args.flags), callback(args.callback),
              user_data(args.user_data) {
        }

        bool check() {
            if (this->shape.has_value())
                return ImGui::InputTextMultiline(this->label, &this->data, *this->shape,
                                                 this->flags, this->callback, this->user_data);
            else
                return ImGui::InputText(this->label, &this->data, this->flags, this->callback,
                                        this->user_data);
        }

      private:
        std::optional<ImVec2> shape;
        ImGuiInputTextFlags flags;
        ImGuiInputTextCallback callback;
        void* user_data;
    };
} // namespace sauce::ui
