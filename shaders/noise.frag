#version 440
// The preview's television snow: the same maths as the hub's genry_tb.noise shader in the game.
layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;
layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float time;
};
layout(binding = 1) uniform sampler2D source;

void main()
{
    vec4 mid = texture(source, qt_TexCoord0);
    vec2 cell = floor(qt_TexCoord0 * vec2(480.0, 270.0));
    float n = fract(sin(dot(cell + fract(time * 7.0) * 100.0, vec2(12.9898, 78.233))) * 43758.5453);
    float roll = 0.85 + 0.15 * sin(qt_TexCoord0.y * 9.0 - time * 3.0);
    vec3 col = mix(mid.rgb, vec3(n), 0.38) * roll;
    fragColor = vec4(col * mid.a, mid.a) * qt_Opacity;
}
