
#include "grid_layout.hpp"


ui::GridLayout::GridLayout(
                u32 focus_id, // NOLINT(bugprone-easily-swappable-parameters)
                u32 size,
                Direction direction,
                Margin gap,
                std::pair<double, double> margin,
                const Layout& layout,
                bool is_top_level 
        )
            : FocusLayout{ layout, focus_id, FocusOptions{ is_top_level, is_top_level }, is_top_level },
              size{ size },
              direction{ direction },
              gap{ gap },
              margin{ static_cast<u32>(margin.first * layout.get_rect().width()),
                      static_cast<u32>(margin.second * layout.get_rect().height()) } {
        }

[[nodiscard]] u32 ui::GridLayout::total_size() const {
    return size;
}

void ui::GridLayout::render(const ServiceProvider& service_provider) const {
    for (const auto& widget : m_widgets) {
        widget->render(service_provider);
    }
}

helper::BoolWrapper<ui::EventHandleType> ui::GridLayout::handle_event(
        const SDL_Event& event,
        const Window* window
) // NOLINT(readability-function-cognitive-complexity)
{
    helper::BoolWrapper<ui::EventHandleType> handled = handle_focus_change_events(event, window);

    if (handled) {
        return handled;
    }

    if (utils::device_supports_clicks()) {

        if (utils::event_is_click_event(event, utils::CrossPlatformClickEvent::Any)) {

            for (auto& widget : m_widgets) {
                const auto layout = widget->layout();
                if (not handled and utils::is_event_in(window, event, layout.get_rect())) {
                    if (const auto event_result = widget->handle_event(event, window); event_result) {
                        handled = { true, handle_event_result(event_result.get_additional(), widget.get()) };
                        continue;
                    }
                } else {
                    const auto hoverable = as_hoverable(widget.get());
                    if (hoverable.has_value()) {
                        hoverable.value()->on_unhover();
                    }
                }
            }
            return handled;
        }
    }

    return handled;
}


[[nodiscard]] ui::Layout ui::GridLayout::get_layout_for_index(u32 index) {
    if (index >= this->size) {
        throw std::runtime_error("GridLayout is already full");
    }

    const auto start_point = layout().get_rect().top_left;

    u32 x = start_point.x + margin.first;
    u32 y = start_point.y + margin.second;
    u32 width = layout().get_rect().width() - (margin.first * 2);
    u32 height = layout().get_rect().height() - (margin.second * 2);

    if (direction == Direction::Horizontal) {
        const u32 total_margin = this->size <= 1 ? 0 : (this->size - 1) * gap.get_margin();
        width = (layout().get_rect().width() - total_margin - (margin.first * 2)) / this->size;

        const u32 margin_x = index * gap.get_margin();
        const u32 total_width = width * index;
        x += margin_x + total_width;
    } else {
        const u32 total_margin = this->size <= 1 ? 0 : (this->size - 1) * gap.get_margin();
        height = (layout().get_rect().height() - total_margin - (margin.second * 2)) / this->size;

        const u32 margin_y = index * gap.get_margin();
        const u32 total_height = height * index;
        y += margin_y + total_height;
    }


    return AbsolutLayout{
        x,
        y,
        width,
        height,
    };
}