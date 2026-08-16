/*
 * @file modules/qt/src/widgets/viewport/ShaderSources.cpp
 * @brief GLSL sources used by the point-sprite viewport.
 */

#include "widgets/viewport/ShaderSources.hpp"

namespace bltzr_qt {

const char kVertexShader[] = R"GLSL(
#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in float a_mass;
layout(location = 2) in float a_pressure;
layout(location = 3) in float a_temperature;
layout(location = 4) in float a_density;
uniform int u_mode;
uniform vec3 u_camera;
uniform float u_zoom;
uniform float u_point_scale;
uniform vec2 u_viewport;
uniform float u_lod_near;
uniform float u_lod_far;
uniform bool u_lod_enabled;
out float v_pressure;
out float v_temperature;
out float v_mass;
out float v_density;
void rotate3d(vec3 value, vec3 angles, out vec3 result) {
    float cy = cos(angles.x);
    float sy = sin(angles.x);
    float cp = cos(angles.y);
    float sp = sin(angles.y);
    float cr = cos(angles.z);
    float sr = sin(angles.z);
    float x1 = cy * value.x - sy * value.z;
    float z1 = sy * value.x + cy * value.z;
    float y1 = cp * value.y - sp * z1;
    float z2 = sp * value.y + cp * z1;
    result = vec3(cr * x1 - sr * y1, sr * x1 + cr * y1, z2);
}
void main() {
    vec3 base = (u_mode >= 3) ? vec3(0.7853981634, 0.6154797087, 0.0) : vec3(0.0);
    vec3 rotated;
    rotate3d(a_position, base + u_camera, rotated);
    float sx = rotated.x;
    float sy = rotated.y;
    float depth = rotated.z;
    if (u_mode == 1) { sy = rotated.z; depth = rotated.y; }
    if (u_mode == 2) { sx = rotated.y; sy = rotated.z; depth = rotated.x; }
    if (u_mode == 4) {
        float denominator = 40.0 - depth;
        float perspective = 40.0 / denominator;
        if (abs(denominator) < 0.001 || perspective <= 0.0 || perspective > 30.0) {
            gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
            return;
        }
        sx *= perspective;
        sy *= perspective;
    }
    float distanceFromOrigin = length(a_position);
    if (u_lod_enabled && u_mode == 4 && distanceFromOrigin > u_lod_near &&
        (uint(gl_VertexID) % 100u) < uint(clamp((distanceFromOrigin - u_lod_near) /
        max(0.001, u_lod_far - u_lod_near), 0.0, 1.0) * 100.0)) {
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
        return;
    }
    vec2 halfViewport = max(u_viewport * 0.5, vec2(1.0));
    gl_Position = vec4((sx * u_zoom) / halfViewport.x,
                       (sy * u_zoom) / halfViewport.y, clamp(depth / 40.0, -1.0, 1.0), 1.0);
    gl_PointSize = clamp((a_mass > 100.0 ? 2.0 : 1.0) * u_point_scale, 1.0, 32.0);
    v_pressure = a_pressure;
    v_temperature = a_temperature;
    v_mass = a_mass;
    v_density = a_density;
}
)GLSL";

const char kFragmentShader[] = R"GLSL(
#version 330 core
in float v_pressure;
in float v_temperature;
in float v_mass;
in float v_density;
uniform float u_temperature_scale;
uniform float u_pressure_scale;
uniform float u_luminosity;
out vec4 fragColor;
void main() {
    if (v_mass > 100.0) {
        fragColor = vec4(1.0, 0.35, 0.35, u_luminosity > 0.0 ? 1.0 : 0.0);
        return;
    }
    float temperature = clamp(v_temperature / max(0.25, u_temperature_scale), 0.0, 1.0);
    vec3 cold = vec3(0.22, 0.41, 1.0);
    vec3 warm = vec3(1.0, 0.67, 0.35);
    vec3 hot = vec3(1.0, 0.30, 0.26);
    vec3 color = temperature < 0.55
        ? mix(cold, warm, temperature / 0.55)
        : mix(warm, hot, (temperature - 0.55) / 0.45);
    float pressure = clamp(v_pressure / max(0.25, u_pressure_scale), 0.0, 1.0);
    float density = clamp(v_density, 0.0, 1.0);
    if (density > 0.0) {
        vec3 densityColor = mix(vec3(0.14, 0.29, 0.75), vec3(1.0, 0.88, 0.35), density);
        color = mix(color, densityColor, 0.65);
    }
    color *= 0.35 + 0.65 * density;
    float visibility = max(0.55, max(pressure, density));
    fragColor = vec4(color, u_luminosity > 0.0 ? 1.0 : 0.0);
}
)GLSL";

const char* vertexShaderSource()
{
    return kVertexShader;
}

const char* fragmentShaderSource()
{
    return kFragmentShader;
}

} // namespace bltzr_qt
