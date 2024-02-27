#include "grid.hpp"
#include "graphics/renderer.hpp"

#include <spdlog/spdlog.h>

Grid::Grid(const ui::Layout& layout) : ui::Widget{ layout } {

    const u32 total_x_tiles = preview_extends.x * 2 + 2 + width_in_tiles;
    constexpr u32 total_y_tiles = height_in_tiles;

    const u32 tile_size_x = layout.get_rect().width() / total_x_tiles;
    const u32 tile_size_y = layout.get_rect().height() / total_y_tiles;

    m_tile_size = std::min(tile_size_y, tile_size_x);
    spdlog::info(
            "creating grid with a tile size of {}px, {}x{}", m_tile_size, m_tile_size * total_x_tiles,
            m_tile_size * total_y_tiles
    );

    m_fill_rect = ui::get_rectangle_aligned(
            layout, { m_tile_size * total_x_tiles, m_tile_size * total_y_tiles },
            ui::Alignment{ ui::AlignmentHorizontal::Middle, ui::AlignmentVertical::Center }
    );
}

[[nodiscard]] shapes::UPoint Grid::tile_size() const {
    return shapes::UPoint{ m_tile_size, m_tile_size };
}
[[nodiscard]] double Grid::scale_to_original() const {
    return static_cast<double>(m_tile_size) / static_cast<double>(original_tile_size);
}

[[nodiscard]] shapes::UPoint Grid::to_screen_coords(GridPoint grid_coords) const {
    return m_fill_rect.top_left + (grid_coords.cast<u32>() * m_tile_size);
}

void Grid::render(const ServiceProvider& service_provider) const {
    draw_preview_background(service_provider);
    draw_hold_background(service_provider);
    draw_playing_field_background(service_provider);
}

[[nodiscard]] helper::BoolWrapper<ui::EventHandleType> Grid::handle_event(const SDL_Event&, const Window*) {
    return false;
}

void Grid::draw_preview_background(const ServiceProvider& service_provider) const {
    draw_background(
            service_provider,
            GridRect{
                    preview_background_position,
                    preview_background_position + preview_extends - GridPoint{1, 1},
    }
    );
}

void Grid::draw_hold_background(const ServiceProvider& service_provider) const {
    draw_background(
            service_provider,
            GridRect{
                    hold_background_position,
                    hold_background_position + hold_background_extends - GridPoint{1, 1},
    }
    );
}

void Grid::draw_playing_field_background(const ServiceProvider& service_provider) const {
    draw_background(
            service_provider,
            GridRect{
                    grid_position,
                    grid_position + shapes::UPoint{width_in_tiles - 1, height_in_tiles - 1},
    }
    );
}

void Grid::draw_background(const ServiceProvider& service_provider, GridRect grid_rect) const {
    const auto top_left = m_fill_rect.top_left + (grid_rect.top_left.cast<u32>() * m_tile_size);


    const auto bottom_right = top_left + (GridPoint(grid_rect.width(), grid_rect.height()).cast<u32>() * m_tile_size);

    service_provider.renderer().draw_rect_filled(shapes::URect{ top_left, bottom_right }, background_color);

    for (u32 column = 0; column <= grid_rect.width(); ++column) {
        const auto start = top_left + shapes::UPoint{ column * m_tile_size, 0 };
        const auto end = shapes::UPoint{ start.x, start.y + grid_rect.height() * m_tile_size };
        service_provider.renderer().draw_line(start, end, grid_color);
        service_provider.renderer().draw_line(start - shapes::UPoint{ 1, 0 }, end - shapes::UPoint{ 1, 0 }, grid_color);
    }

    for (u32 row = 0; row <= grid_rect.height(); ++row) {
        const auto start = top_left + shapes::UPoint{ 0, row * m_tile_size };
        const auto end = shapes::UPoint{ bottom_right.x, start.y };
        service_provider.renderer().draw_line(start, end, grid_color);
        service_provider.renderer().draw_line(start - shapes::UPoint{ 0, 1 }, end - shapes::UPoint{ 0, 1 }, grid_color);
    }

    const auto outline_top_left = top_left - shapes::UPoint{ 2, 2 };
    const auto outline_bottom_right = shapes::UPoint{
        top_left.x + grid_rect.width() * m_tile_size + 1,
        top_left.y + grid_rect.height() * m_tile_size + 1,
    };

    const auto outline_rect = shapes::URect{
        outline_top_left,
        outline_bottom_right,
    };
    service_provider.renderer().draw_rect_outline(outline_rect, border_color);
}
