#pragma once

#include <core/helper/types.hpp>

enum class Rotation : u8 {
    North = 0,
    East,
    South,
    West,
    LastRotation = West,
};

Rotation& operator++(Rotation& rotation);

Rotation& operator--(Rotation& rotation);

Rotation operator+(Rotation rotation, i8 offset);

Rotation operator-(Rotation rotation, i8 offset);
