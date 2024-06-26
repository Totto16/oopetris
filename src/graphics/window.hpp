#pragma once

#include <core/helper/point.hpp>

#include "graphics/rect.hpp"
#include "helper/message_box.hpp"
#include "sdl_context.hpp"

#include <SDL.h>
#include <string>

enum class WindowPosition {
    Centered = SDL_WINDOWPOS_CENTERED,
    Undefined = SDL_WINDOWPOS_UNDEFINED,
};

struct Window final {
private:
    SDL_Window* m_window;
    SdlContext m_context;

public:
    Window(const std::string& title, WindowPosition position, u32 width, u32 height);
    Window(const std::string& title, u32 x, u32 y, u32 width, u32 height);
    Window(const std::string& title, WindowPosition position);
    Window(const std::string& title, u32 x, u32 y);
    Window(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(const Window&) = delete;
    Window& operator=(Window&&) = delete;
    ~Window();

    [[nodiscard]] shapes::UPoint size() const;

    [[nodiscard]] SDL_Window* get_sdl_window() const;

    [[nodiscard]] const SdlContext& context() const;

    [[nodiscard]] shapes::URect screen_rect() const;

    void show_simple(helper::MessageBox::Type type, const std::string& title, const std::string& content) const;
};
