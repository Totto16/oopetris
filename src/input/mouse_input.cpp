

#include "mouse_input.hpp"
#include "graphics/point.hpp"
#include "helper/optional.hpp"
#include "input/input.hpp"


input::MouseInput::MouseInput() : PointerInput("mouse") { }

[[nodiscard]] SDL_Event input::MouseInput::offset_pointer_event(const SDL_Event& event, const shapes::IPoint& point)
        const {

    auto new_event = event;

    switch (event.type) {
        case SDL_MOUSEMOTION:
            new_event.motion.x = event.motion.x + point.x;
            new_event.motion.y = event.motion.y + point.y;
            break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            new_event.button.x = event.button.x + point.x;
            new_event.button.y = event.button.y + point.y;
            break;
        default:
            throw std::runtime_error("Tried to offset event, that is no pointer event: in Mouse Input");
    }

    return new_event;
}


[[nodiscard]] helper::optional<input::NavigationEvent>
input::MouseInput::get_navigation_event(const SDL_Event& /*event*/) const {
    return helper::nullopt;
}

[[nodiscard]] std::string input::MouseInput::describe_navigation_event(NavigationEvent /*event*/) const {
    throw std::runtime_error("not supported");
}

[[nodiscard]] helper::optional<input::PointerEventHelper> input::MouseInput::get_pointer_event(const SDL_Event& event
) const {

    auto pointer_event = input::PointerEvent::PointerUp;

    switch (event.type) {
        case SDL_MOUSEMOTION:
            return input::PointerEventHelper{
                shapes::IPoint{ event.motion.x, event.motion.y },
                input::PointerEvent::Motion
            };
        case SDL_MOUSEBUTTONDOWN:
            pointer_event = input::PointerEvent::PointerDown;
            break;
        case SDL_MOUSEBUTTONUP:
            break;
        default:
            return helper::nullopt;
    }

    if (event.button.button != SDL_BUTTON_LEFT) {
        return helper::nullopt;
    }

    const shapes::IPoint pos{ event.button.x, event.button.y };

    return input::PointerEventHelper{ pos, pointer_event };
}
