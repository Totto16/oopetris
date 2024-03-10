#include "renderer.hpp"

//TODO: assert return values of all sdl functions

Renderer::Renderer(Window& window, const VSync v_sync)
    : m_renderer{ SDL_CreateRenderer(
            window.get_sdl_window(),
            -1,
            (v_sync == VSync::Enabled ? SDL_RENDERER_PRESENTVSYNC : 0) | SDL_RENDERER_TARGETTEXTURE
                    | SDL_RENDERER_ACCELERATED
    ) } {
    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
}

Renderer::~Renderer() {
    SDL_DestroyRenderer(m_renderer);
}

void Renderer::set_draw_color(const Color& color) const {
    SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
}

void Renderer::clear(const Color& clear_color) const {
    set_draw_color(clear_color);
    int result = SDL_RenderClear(m_renderer);
    ASSERT(result == 0 && "render clear was executed without error");
}

void Renderer::present() const {
    SDL_RenderPresent(m_renderer);
}

Texture Renderer::load_image(const std::string& image_path) const {
    return Texture::from_image(m_renderer, image_path);
}

Texture Renderer::prerender_text(
        const std::string& text,
        const Font& font,
        const Color& color,
        RenderType render_type,
        const Color& background_color
) const {
    return Texture::prerender_text(m_renderer, text, font, color, render_type, background_color);
}

Texture Renderer::get_texture_for_render_target(const shapes::UPoint& size) const {


    const auto supported = SDL_RenderTargetSupported(m_renderer);

    if (supported == SDL_FALSE) {
        throw std::runtime_error("SDL does not support a target renderer, but we need one!");
    }

    return Texture::get_for_render_target(m_renderer, size);
}


void Renderer::set_render_target(const Texture& texture) const {
    texture.set_as_render_target(m_renderer);
}

void Renderer::reset_render_target() const {
    const auto result = SDL_SetRenderTarget(m_renderer, nullptr);
    if (result < 0) {
        throw std::runtime_error(fmt::format("Failed to set render texture target with error: {}", SDL_GetError()));
    }
}
