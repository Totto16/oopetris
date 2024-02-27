#include "mino_stack.hpp"

#include "grid.hpp"

void MinoStack::clear_row_and_let_sink(u8 row) {
    m_minos.erase(
            std::remove_if(m_minos.begin(), m_minos.end(), [&](const Mino& mino) { return mino.position().y == row; }),
            m_minos.end()
    );
    for (Mino& mino : m_minos) {
        if (mino.position().y < row) {
            ++mino.position().y;
        }
    }
}

[[nodiscard]] bool MinoStack::is_empty(GridPoint coordinates) const {
    for (const Mino& mino : m_minos) { // NOLINT(readability-use-anyofallof)
        if (mino.position() == coordinates) {
            return false;
        }
    }
    return true;
}

void MinoStack::set(GridPoint coordinates, TetrominoType type) {
    const Mino to_insert = Mino{ coordinates, type };
    for (Mino& current : m_minos) {
        if (current.position() == coordinates) {
            current = to_insert;
            return;
        }
    }
    m_minos.push_back(to_insert);
}

void MinoStack::draw_minos(
        const ServiceProvider& service_provider,
        const double original_scale,
        const ScreenCordsFunction& to_screen_coords,
        const shapes::UPoint& tile_size
) const {
    for (const auto& mino : m_minos) {
        mino.render(service_provider, MinoTransparency::Solid, original_scale, to_screen_coords, tile_size);
    }
}

std::ostream& operator<<(std::ostream& ostream, const MinoStack& mino_stack) {
    ostream << "MinoStack(\n";
    for (u8 y = 0; y < Grid::height_in_tiles; ++y) {
        for (u8 x = 0; x < Grid::width_in_tiles; ++x) {
            const auto find_iterator =
                    std::find_if(mino_stack.minos().cbegin(), mino_stack.minos().cend(), [&](const auto& mino) {
                        return mino.position() == shapes::AbstractPoint<u8>{ x, y };
                    });
            const auto found = (find_iterator != mino_stack.minos().cend());
            if (found) {
                ostream << magic_enum::enum_name(find_iterator->type());
            } else {
                ostream << " ";
            }
        }
        ostream << "\n";
    }

    ostream << ")";
    return ostream;
}
