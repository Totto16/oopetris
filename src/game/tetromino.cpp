
#include "tetromino.hpp"

[[nodiscard]] TetrominoType Tetromino::type() const {
    return m_type;
}

[[nodiscard]] Rotation Tetromino::rotation() const {
    return m_rotation;
}

void Tetromino::render(
        const ServiceProvider& service_provider,
        MinoTransparency transparency,
        const double original_scale,
        const ScreenCordsFunction& to_screen_coords,
        const shapes::UPoint& tile_size,
        const GridPoint& offset
) const {
    for (const auto& mino : m_minos) {
        mino.render(service_provider, transparency, original_scale, to_screen_coords, tile_size, offset);
    }
}

void Tetromino::rotate_right() {
    ++m_rotation;
    refresh_minos();
}

void Tetromino::rotate_left() {
    --m_rotation;
    refresh_minos();
}

void Tetromino::move_down() {
    move({ 0, 1 });
}

void Tetromino::move_left() {
    move({ -1, 0 });
}

void Tetromino::move_right() {
    move({ 1, 0 });
}

void Tetromino::move(const shapes::AbstractPoint<i8> offset) {
    // this looks weird but silently asserts, that the final point is not negative
    m_position = (m_position.cast<i8>() + offset).cast<u8>();
    refresh_minos();
}

[[nodiscard]] const std::array<Mino, 4>& Tetromino::minos() const {
    return m_minos;
}


void Tetromino::refresh_minos() {
    m_minos = create_minos(m_position, m_rotation, m_type);
}

Tetromino::Pattern Tetromino::get_pattern(TetrominoType type, Rotation rotation) {
    return tetrominos.at(static_cast<usize>(type)).at(static_cast<usize>(rotation));
}


std::array<Mino, 4> Tetromino::create_minos(GridPoint position, Rotation rotation, TetrominoType type) {
    return std::array<Mino, 4>{
        Mino{position + get_pattern(type, rotation).at(0), type},
        Mino{position + get_pattern(type, rotation).at(1), type},
        Mino{position + get_pattern(type, rotation).at(2), type},
        Mino{position + get_pattern(type, rotation).at(3), type},
    };
}
