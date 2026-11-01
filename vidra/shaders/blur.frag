layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 oColor;
uniform sampler2D uSrc;
layout(std140) uniform BlurU { vec2 dir; float sigma; float pad; } B;
void main() {
  vec4 sum = vec4(0.0);
  float wsum = 0.0;
  for (int i = -9; i <= 9; ++i) {
    float w = exp(-0.5 * float(i * i) / max(B.sigma * B.sigma, 1e-3));
    sum += texture(uSrc, vUV + B.dir * float(i) * 1.4) * w;
    wsum += w;
  }
  oColor = sum / wsum;
}
