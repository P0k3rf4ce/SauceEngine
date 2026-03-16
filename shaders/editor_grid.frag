#version 450

layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
} ubo;

layout(location = 0) in vec3 nearPoint;
layout(location = 1) in vec3 farPoint;

layout(location = 0) out vec4 outColor;

vec4 grid(vec3 fragPos3D, float scale) {
    vec2 coord = fragPos3D.xy * scale; // Z-up, so grid is on XY plane (Z=0)
    vec2 derivative = fwidth(coord);
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
    float line = min(grid.x, grid.y);
    float minimumz = min(derivative.y, 1.0);
    float minimumx = min(derivative.x, 1.0);
    vec4 color = vec4(0.4, 0.4, 0.4, 1.0 - min(line, 1.0));
    // X axis (red) - along X when Y is near 0
    if(fragPos3D.y > -0.1 / scale * minimumz && fragPos3D.y < 0.1 / scale * minimumz)
        color = vec4(1.0, 0.3, 0.3, 1.0 - min(grid.y, 1.0));
    // Y axis (green) - along Y when X is near 0
    if(fragPos3D.x > -0.1 / scale * minimumx && fragPos3D.x < 0.1 / scale * minimumx)
        color = vec4(0.3, 1.0, 0.3, 1.0 - min(grid.x, 1.0));
    return color;
}

float computeDepth(vec3 pos) {
    vec4 clip_space_pos = ubo.proj * ubo.view * vec4(pos, 1.0);
    return clip_space_pos.z / clip_space_pos.w;
}

float computeLinearDepth(vec3 pos) {
    float near = 0.1;
    float far = 1000.0;
    vec4 clip_space_pos = ubo.proj * ubo.view * vec4(pos, 1.0);
    float clip_space_depth = (clip_space_pos.z / clip_space_pos.w);
    float linearDepth = (2.0 * near * far) / (far + near - clip_space_depth * (far - near));
    return linearDepth / far; // normalize
}

void main() {
    // Ray-plane intersection with Z=0 plane
    float t = -nearPoint.z / (farPoint.z - nearPoint.z);

    vec3 fragPos3D = nearPoint + t * (farPoint - nearPoint);

    gl_FragDepth = computeDepth(fragPos3D);

    float linearDepth = computeLinearDepth(fragPos3D);
    // Gentle fade: fully visible up close, fades out over distance
    float fading = max(0.0, 1.0 - linearDepth * 1.5);

    // Only render if the intersection is valid (t > 0) and in front of camera
    outColor = grid(fragPos3D, 1.0) * float(t > 0.0);
    outColor.a *= fading;
}
