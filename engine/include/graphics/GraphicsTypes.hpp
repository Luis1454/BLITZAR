#ifndef GRAVITY_ENGINE_INCLUDE_GRAPHICS_GRAPHICSTYPES_HPP_
#define GRAVITY_ENGINE_INCLUDE_GRAPHICS_GRAPHICSTYPES_HPP_
#include <array>
#include <cstdint>
namespace grav {
struct Point2D {
    float x;
    float y;
};
struct Rect2D {
    float x;
    float y;
    float width;
    float height;
};
struct ColorRGBA {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
    std::uint8_t a;
};
} // namespace grav
#endif // GRAVITY_ENGINE_INCLUDE_GRAPHICS_GRAPHICSTYPES_HPP_
