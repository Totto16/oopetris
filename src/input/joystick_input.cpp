

#include "joystick_input.hpp"
#include "SDL_stdinc.h"
#include "helper/expected.hpp"
#include "helper/optional.hpp"
#include "input/input.hpp"

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <spdlog/spdlog.h>


joystick::GUID::GUID() : m_guid{} { }
joystick::GUID::GUID(const SDL_GUID& data) : m_guid{} {
    std::copy(std::begin(data.data), std::end(data.data), std::begin(m_guid));
}

[[nodiscard]] bool joystick::GUID::operator==(const GUID& other) const {
    return this->m_guid == other.m_guid;
}

[[nodiscard]] joystick::GUID::operator std::string() const {
    return fmt::format("{}", fmt::join(m_guid, ":"));
}


input::JoystickInput::JoystickInput(SDL_Joystick* joystick, SDL_JoystickID instance_id, const std::string& name)
    : input::Input{ name, input::InputType::JoyStick },
      m_joystick{ joystick },
      m_instance_id{ instance_id } { }


input::JoystickInput::~JoystickInput() {
    SDL_JoystickClose(m_joystick);
}


[[nodiscard]] helper::optional<std::unique_ptr<input::JoystickInput>> input::JoystickInput::get_joystick_by_guid(
        const joystick::GUID& guid,
        SDL_Joystick* joystick,
        SDL_JoystickID instance_id
) {
#if defined(__CONSOLE__)
#if defined(__SWITCH__)
    if (guid == SwitchJoystickInput_Type1::guid) {
        return std::make_unique<SwitchJoystickInput_Type1>(joystick, instance_id);
    }
#elif defined(__3DS__)
    if (guid == _3DSJoystickInput_Type1::guid) {
        return std::make_unique<_3DSJoystickInput_Type1>(joystick, instance_id);
    }

#endif
#endif

    UNUSED(guid);
    UNUSED(joystick);
    UNUSED(instance_id);

    return helper::nullopt;
}


[[nodiscard]] helper::expected<std::unique_ptr<input::JoystickInput>, std::string>
input::JoystickInput::get_by_device_index(int device_index) {


    auto* joystick = SDL_JoystickOpen(device_index);

    if (joystick == nullptr) {
        return helper::unexpected<std::string>{
            fmt::format("Failed to get joystick at device index {}: {}", device_index, SDL_GetError())
        };
    }

    //TODO: add support for gamecontrollers (SDL_IsGameController)

    const auto instance_id = SDL_JoystickInstanceID(joystick);

    if (instance_id < 0) {
        return helper::unexpected<std::string>{ fmt::format("Failed to get joystick instance id: {}", SDL_GetError()) };
    }

    std::string name = "unknown name";
    const auto* char_name = SDL_JoystickName(joystick);

    if (char_name != nullptr) {
        name = char_name;
    }


    const auto guid = joystick::GUID{ SDL_JoystickGetGUID(joystick) };

    if (guid == joystick::GUID{}) {
        return helper::unexpected<std::string>{ fmt::format("Failed to get joystick GUID: {}", SDL_GetError()) };
    }

    auto joystick_input = JoystickInput::get_joystick_by_guid(guid, joystick, instance_id);

    if (joystick_input.has_value()) {
        return std::move(joystick_input.value());
    }

    return helper::unexpected<std::string>{
        fmt::format("Failed to get joystick model by GUID {} We don't support this joystick yet", guid)
    };
}


input::JoyStickInputManager::JoyStickInputManager() {


    //initialize joystick input, this needs to call some sdl things

    const auto result = SDL_InitSubSystem(SDL_INIT_JOYSTICK);

    if (result != 0) {
        spdlog::warn("Failed to initialize the joystick system: {}", SDL_GetError());
        return;
    }


    const auto enable_result = SDL_JoystickEventState(SDL_ENABLE);

    if (enable_result != 1) {
        const auto* const error = enable_result == 0 ? "it was disabled" : SDL_GetError();
        spdlog::warn("Failed to set JoystickEventState (automatic polling by SDL): {}", error);

        return;
    }


    const auto allow_background_events_result = SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

    if (allow_background_events_result != SDL_TRUE) {
        spdlog::warn("Failed to set the JOYSTICK_ALLOW_BACKGROUND_EVENTS hint: {}", SDL_GetError());

        return;
    }


    const auto num_of_joysticks = SDL_NumJoysticks();

    if (num_of_joysticks < 0) {
        spdlog::warn("Failed to get number of joysticks: {}", SDL_GetError());
        return;
    }

    for (auto i = 0; i < num_of_joysticks; ++i) {

        auto joystick = JoystickInput::get_by_device_index(i);
        if (joystick.has_value()) {
            m_inputs.push_back(std::move(joystick.value()));
        } else {
            spdlog::warn("Failed to configure joystick: {}", joystick.error());
        }
    }
}


[[nodiscard]] const std::vector<std::unique_ptr<input::JoystickInput>>& input::JoyStickInputManager::inputs() const {
    return m_inputs;
}


[[nodiscard]] helper::optional<input::NavigationEvent> input::JoyStickInputManager::get_navigation_event(
        const SDL_Event& event
) const {
    for (const auto& input : m_inputs) {

        if (const auto navigation_event = input->get_navigation_event(event); navigation_event.has_value()) {
            return navigation_event;
        }
    }

    return helper::nullopt;
}

[[nodiscard]] helper::optional<input::PointerEventHelper> input::JoyStickInputManager::get_pointer_event(
        const SDL_Event& event
) const {
    for (const auto& input : m_inputs) {
        if (const auto pointer_input = utils::is_child_class<input::PointerInput>(input); pointer_input.has_value()) {
            if (const auto pointer_event = pointer_input.value()->get_pointer_event(event); pointer_event.has_value()) {
                return pointer_event;
            }
        }
    }

    return helper::nullopt;
}

[[nodiscard]] bool input::JoyStickInputManager::process_special_inputs(const SDL_Event& event) {

    switch (event.type) {
        case SDL_JOYDEVICEADDED: {
            const auto device_id = event.jdevice.which;
            auto joystick = JoystickInput::get_by_device_index(device_id);
            if (joystick.has_value()) {
                m_inputs.push_back(std::move(joystick.value()));
            } else {
                spdlog::warn("Failed to add newly added joystick: {}", joystick.error());
            }
            return true;
        }
        case SDL_JOYDEVICEREMOVED: {
            const auto instance_id = event.jdevice.which;
            for (auto it = m_inputs.cbegin(); it != m_inputs.end(); it++) {

                if ((*it)->instance_id() == instance_id) {
                    m_inputs.erase(it);
                    return true;
                }
            }

            spdlog::warn("Failed to remove removed joystick from internal joystick vector");

            return true;
        }
        default:
            return false;
    }
    //TODO
}


#if defined(__CONSOLE__)
#include "console_input.hpp"

#if defined(__SWITCH__)

input::SwitchJoystickInput_Type1::SwitchJoystickInput_Type1(
        SDL_Joystick* joystick,
        SDL_JoystickID instance_id,
        const std::string& name
)
    : JoystickInput{ joystick, instance_id, name } { }


[[nodiscard]] helper::optional<NavigationEvent> input::SwitchJoystickInput_Type1::get_navigation_event(
        const SDL_Event& event
) const {

    //TODO handle SDL_JOYAXISMOTION


    if (event.type == SDL_JOYBUTTONDOWN) {

        if (event.jbutton.which != m_id) {
            return helper::nullopt;
        }


        switch (event.jbutton.button) {
            case JOYCON_A:
                return NavigationEvent::OK;
            case JOYCON_DPAD_DOWN:
            case JOYCON_LDPAD_DOWN:
            case JOYCON_RDPAD_DOWN:
                return NavigationEvent::DOWN;
            case JOYCON_DPAD_UP:
            case JOYCON_LDPAD_UP:
            case JOYCON_RDPAD_UP:
                return NavigationEvent::UP;
            case JOYCON_DPAD_LEFT:
            case JOYCON_LDPAD_LEFT:
            case JOYCON_RDPAD_LEFT:
                return NavigationEvent::LEFT;
            case JOYCON_DPAD_RIGHT:
            case JOYCON_LDPAD_RIGHT:
            case JOYCON_RDPAD_RIGHT:
                return NavigationEvent::RIGHT;
            case JOYCON_MINUS:
                return NavigationEvent::BACK;


                //note, that  NavigationEvent::TAB is not supported
        }
    }

    //TODO: handle SDL_JOYAXISMOTION


    return helper::nullopt;
}

#elif defined(__3DS__)

input::_3DSJoystickInput_Type1::_3DSJoystickInput_Type1(
        SDL_Joystick* joystick,
        SDL_JoystickID instance_id,
        const std::string& name
)
    : JoystickInput{ joystick, instance_id, name } { }


[[nodiscard]] helper::optional<NavigationEvent> input::_3DSJoystickInput_Type1::get_navigation_event(
        const SDL_Event& event
) const {


    if (event.type == SDL_JOYBUTTONDOWN) {

        if (event.jbutton.which != m_id) {
            return helper::nullopt;
        }


        switch (event.jbutton.button) {
            case JOYCON_A:
                return NavigationEvent::OK;
            case JOYCON_DPAD_DOWN:
            case JOYCON_CSTICK_DOWN:
            case JOYCON_CPAD_DOWN:
                return NavigationEvent::DOWN;
            case JOYCON_DPAD_UP:
            case JOYCON_CSTICK_UP:
            case JOYCON_CPAD_UP:
                return NavigationEvent::UP;
            case JOYCON_DPAD_LEFT:
            case JOYCON_CSTICK_LEFT:
            case JOYCON_CPAD_LEFT:
                return NavigationEvent::LEFT;
            case JOYCON_DPAD_RIGHT:
            case JOYCON_CSTICK_RIGHT:
            case JOYCON_CPAD_RIGHT:
                return NavigationEvent::RIGHT;
            case JOYCON_X:
                return NavigationEvent::BACK;

                //note, that  NavigationEvent::TAB is not supported
        }
    }

    //TODO: handle SDL_JOYAXISMOTION


    return helper::nullopt;
}

#endif

void JoystickGameInput::handle_event(const SDL_Event& event, const Window*) {
    m_event_buffer.push_back(event);
}

void JoystickGameInput::update(SimulationStep simulation_step_index) {
    for (const auto& event : m_event_buffer) {
        const auto input_event = sdl_event_to_input_event(event);
        if (input_event.has_value()) {
            Input::handle_event(*input_event, simulation_step_index);
        }
    }
    m_event_buffer.clear();

    Input::update(simulation_step_index);
}

#if defined(__SWITCH__)

// game_input uses Input to handle events, but stores teh config settings for teh specific button

helper::optional<InputEvent> JoystickSwitchGameInput_Type1::sdl_event_to_input_event(const SDL_Event& event) const {
    if (event.type == SDL_JOYBUTTONDOWN) {
        //TODO: use switch case
        const auto button = event.jbutton.button;
        if (button == JOYCON_DPAD_LEFT) {
            return InputEvent::RotateLeftPressed;
        }
        if (button == JOYCON_DPAD_RIGHT) {
            return InputEvent::RotateRightPressed;
        }
        if (button == JOYCON_LDPAD_DOWN or button == JOYCON_RDPAD_DOWN) {
            return InputEvent::MoveDownPressed;
        }
        if (button == JOYCON_LDPAD_LEFT or button == JOYCON_RDPAD_LEFT) {
            return InputEvent::MoveLeftPressed;
        }
        if (button == JOYCON_LDPAD_RIGHT or button == JOYCON_RDPAD_RIGHT) {
            return InputEvent::MoveRightPressed;
        }
        if (button == JOYCON_X) {
            return InputEvent::DropPressed;
        }
        if (button == JOYCON_B) {
            return InputEvent::HoldPressed;
        }
    } else if (event.type == SDL_JOYBUTTONUP) {
        const auto button = event.jbutton.button;
        if (button == JOYCON_DPAD_LEFT) {
            return InputEvent::RotateLeftReleased;
        }
        if (button == JOYCON_DPAD_RIGHT) {
            return InputEvent::RotateRightReleased;
        }
        if (button == JOYCON_LDPAD_DOWN or button == JOYCON_RDPAD_DOWN) {
            return InputEvent::MoveDownReleased;
        }
        if (button == JOYCON_LDPAD_LEFT or button == JOYCON_RDPAD_LEFT) {
            return InputEvent::MoveLeftReleased;
        }
        if (button == JOYCON_LDPAD_RIGHT or button == JOYCON_RDPAD_RIGHT) {
            return InputEvent::MoveRightReleased;
        }
        if (button == JOYCON_X) {
            return InputEvent::DropReleased;
        }
        if (button == JOYCON_B) {
            return InputEvent::HoldReleased;
        }
    }
    return helper::nullopt;
}
#elif defined(__3DS__)


helper::optional<InputEvent> JoystickInput::sdl_event_to_input_event(const SDL_Event& event) const {
    if (event.type == SDL_JOYBUTTONDOWN) {
        const auto button = event.jbutton.button;
        if (button == JOYCON_L) {
            return InputEvent::RotateLeftPressed;
        }
        if (button == JOYCON_R) {
            return InputEvent::RotateRightPressed;
        }
        if (button == JOYCON_DPAD_DOWN or button == JOYCON_CSTICK_DOWN) {
            return InputEvent::MoveDownPressed;
        }
        if (button == JOYCON_DPAD_LEFT or button == JOYCON_CSTICK_LEFT) {
            return InputEvent::MoveLeftPressed;
        }
        if (button == JOYCON_DPAD_RIGHT or button == JOYCON_CSTICK_RIGHT) {
            return InputEvent::MoveRightPressed;
        }
        if (button == JOYCON_A) {
            return InputEvent::DropPressed;
        }
        if (button == JOYCON_B) {
            return InputEvent::HoldPressed;
        }
    } else if (event.type == SDL_JOYBUTTONUP) {
        const auto button = event.jbutton.button;
        if (button == JOYCON_L) {
            return InputEvent::RotateLeftReleased;
        }
        if (button == JOYCON_R) {
            return InputEvent::RotateRightReleased;
        }
        if (button == JOYCON_DPAD_DOWN or button == JOYCON_CSTICK_DOWN) {
            return InputEvent::MoveDownReleased;
        }
        if (button == JOYCON_DPAD_LEFT or button == JOYCON_CSTICK_LEFT) {
            return InputEvent::MoveLeftReleased;
        }
        if (button == JOYCON_DPAD_RIGHT or button == JOYCON_CSTICK_RIGHT) {
            return InputEvent::MoveRightReleased;
        }
        if (button == JOYCON_A) {
            return InputEvent::DropReleased;
        }
        if (button == JOYCON_B) {
            return InputEvent::HoldReleased;
        }
    }
    return helper::nullopt;
}
#endif


#endif


[[nodiscard]] helper::expected<bool, std::string> input::JoystickSettings::validate() const {

    const std::vector<std::string> to_use{ rotate_left, rotate_right, move_left, move_right,   move_down,
                                           drop,        hold,         pause,     open_settings };

    return has_unique_members(to_use);
}


std::string json_helper::get_key_from_object(const nlohmann::json& j, const std::string& name) {

    auto context = j.at(name);

    std::string input;
    context.get_to(input);


    return input;
}
