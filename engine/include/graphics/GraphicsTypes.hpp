// File: engine/include/graphics/GraphicsTypes.hpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#ifndef GRAVITY_ENGINE_INCLUDE_GRAPHICS_GRAPHICSTYPES_HPP_
#define GRAVITY_ENGINE_INCLUDE_GRAPHICS_GRAPHICSTYPES_HPP_
#include <array>
#include <cstdint>

namespace grav {
/// Description: Defines the Point2D data or behavior contract.
struct Point2D {
    float x;
    float y;
};

/// Description: Defines the Rect2D data or behavior contract.
struct Rect2D {
    float x;
    float y;
    float width;
    float height;
};

/// Description: Defines the ColorRGBA data or behavior contract.
struct ColorRGBA {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
    std::uint8_t a;
};
} // namespace grav
#endif // GRAVITY_ENGINE_INCLUDE_GRAPHICS_GRAPHICSTYPES_HPP_
