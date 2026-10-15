#version 440
// The preview's VHS tape: the same maths as the hub's genry_tb.vhs shader in the game.
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
    vec2 uv = qt_TexCoord0;
    float bar = fract(uv.y + time * 0.07);
    float band = smoothstep(0.0, 0.02, bar) * (1.0 - smoothstep(0.02, 0.07, bar));
    uv.x += sin(uv.y * 40.0 + time * 6.0) * 0.0012 + sin(uv.y * 3.0 + time * 1.3) * 0.0015 + band * 0.012;
    vec4 mid = texture(source, uv);
    vec3 col = vec3(texture(source, uv + vec2(0.0025, 0.0)).r, mid.g, texture(source, uv - vec2(0.0025, 0.0)).b);
    float line = 0.90 + 0.10 * step(0.5, fract(uv.y * 270.0));
    float n = fract(sin(dot(uv + fract(time), vec2(12.9898, 78.233))) * 43758.5453);
    col = col * line + (n - 0.5) * 0.09 + band * 0.08;
    fragColor = vec4(col * mid.a, mid.a) * qt_Opacity;
}
