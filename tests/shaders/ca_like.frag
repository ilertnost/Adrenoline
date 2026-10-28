#version 430
// Shape similar to a CoreAnimation layer composite: sample, tint, premultiply.
// Used as the first GLSL -> NIR probe for the Adrenoline seam.
uniform sampler2D uTex;
uniform vec4 uColor;
in vec2 vUV;
out vec4 fragColor;

void main() {
    vec4 t = texture(uTex, vUV);
    vec3 src = t.rgb * uColor.rgb;
    float a = t.a * uColor.a;
    fragColor = vec4(src * a, a);
}
