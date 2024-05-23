#pragma once

#include "SDL_keycode.h"
#include "game_input.hpp"
#include "helper/expected.hpp"
#include "helper/parse_json.hpp"
#include "input.hpp"
#include "manager/event_dispatcher.hpp"
#include "manager/sdl_key.hpp"


#include <stdexcept>


namespace input {


    struct KeyboardInput : Input {

    public:
        KeyboardInput();

        [[nodiscard]] helper::optional<NavigationEvent> get_navigation_event(const SDL_Event& event) const override;

        [[nodiscard]] std::string describe_navigation_event(NavigationEvent event) const override;
    };


    struct KeyboardSettings {
        sdl::Key rotate_left;
        sdl::Key rotate_right;
        sdl::Key move_left;
        sdl::Key move_right;
        sdl::Key move_down;
        sdl::Key drop;
        sdl::Key hold;

        sdl::Key pause;
        sdl::Key open_settings;


        [[nodiscard]] helper::expected<bool, std::string> validate() const;

        [[nodiscard]] static KeyboardSettings default_settings() {
            return KeyboardSettings{ .rotate_left = sdl::Key{ SDLK_LEFT },
                                     .rotate_right = sdl::Key{ SDLK_RIGHT },
                                     .move_left = sdl::Key{ SDLK_a },
                                     .move_right = sdl::Key{ SDLK_d },
                                     .move_down = sdl::Key{ SDLK_s },
                                     .drop = sdl::Key{ SDLK_w },
                                     .hold = sdl::Key{ SDLK_TAB },
                                     .pause = sdl::Key{ SDLK_SPACE },
                                     .open_settings = sdl::Key{ SDLK_e } };
        }
    };


    struct KeyboardGameInput : public GameInput, public EventListener {
    private:
        KeyboardSettings m_settings;
        std::vector<SDL_Event> m_event_buffer;
        EventDispatcher* m_event_dispatcher;

    public:
        KeyboardGameInput(const KeyboardSettings& settings, EventDispatcher* event_dispatcher);

        ~KeyboardGameInput() override;

        KeyboardGameInput(const KeyboardGameInput& input) = delete;
        [[nodiscard]] KeyboardGameInput& operator=(const KeyboardGameInput& input) = delete;

        KeyboardGameInput(KeyboardGameInput&& input) noexcept;
        [[nodiscard]] KeyboardGameInput& operator=(KeyboardGameInput&& input) noexcept;


        void handle_event(const SDL_Event& event) override;

        void update(SimulationStep simulation_step_index) override;

        [[nodiscard]] helper::optional<MenuEvent> get_menu_event(const SDL_Event& event) const override;

        [[nodiscard]] std::string describe_menu_event(MenuEvent event) const override;

    private:
        [[nodiscard]] helper::optional<InputEvent> sdl_event_to_input_event(const SDL_Event& event) const;
    };
} // namespace input


namespace json_helper {


    [[nodiscard]] sdl::Key get_key(const nlohmann::json& obj, const std::string& name);

} // namespace json_helper


namespace nlohmann {
    template<>
    struct adl_serializer<input::KeyboardSettings> {
        static input::KeyboardSettings from_json(const json& obj) {

            ::json::check_for_no_additional_keys(
                    obj, { "type", "rotate_left", "rotate_right", "move_left", "move_right", "move_down", "drop",
                           "hold", "menu" }
            );

            const auto rotate_left = json_helper::get_key(obj, "rotate_left");
            const auto rotate_right = json_helper::get_key(obj, "rotate_right");
            const auto move_left = json_helper::get_key(obj, "move_left");
            const auto move_right = json_helper::get_key(obj, "move_right");
            const auto move_down = json_helper::get_key(obj, "move_down");
            const auto drop = json_helper::get_key(obj, "drop");
            const auto hold = json_helper::get_key(obj, "hold");

            const auto& menu = obj.at("menu");

            ::json::check_for_no_additional_keys(menu, { "pause", "open_settings" });

            const auto pause = json_helper::get_key(menu, "pause");
            const auto open_settings = json_helper::get_key(menu, "open_settings");

            auto settings = input::KeyboardSettings{ .rotate_left = rotate_left,
                                                     .rotate_right = rotate_right,
                                                     .move_left = move_left,
                                                     .move_right = move_right,
                                                     .move_down = move_down,
                                                     .drop = drop,
                                                     .hold = hold,
                                                     .pause = pause,
                                                     .open_settings = open_settings };

            const auto is_valid = settings.validate();
            if (not is_valid.has_value()) {
                throw std::runtime_error(is_valid.error());
            }

            return settings;
        }

        static void to_json(json& obj, const input::KeyboardSettings& settings) {
            obj = nlohmann::json{
                { "rotate_left", settings.rotate_left.to_string() },
                { "rotate_right", settings.rotate_right.to_string() },
                { "move_left", settings.move_left.to_string() },
                { "move_right", settings.move_right.to_string() },
                { "move_down", settings.move_down.to_string() },
                { "drop", settings.drop.to_string() },
                { "hold", settings.hold.to_string() },
                {
                 "menu", nlohmann::json{
                                { "pause", settings.pause.to_string() },
                                { "open_settings", settings.open_settings.to_string() },
                        }, }
            };
        }
    };
} // namespace nlohmann
