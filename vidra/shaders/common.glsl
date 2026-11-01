float sdSquircle(vec2 p, vec2 b, float n) {
  vec2 s = abs(p) / max(b, vec2(1e-3));
  float f = pow(s.x, n) + pow(s.y, n) - 1.0;
  vec2 g = n * pow(max(s, vec2(1e-4)), vec2(n - 1.0)) / max(b, vec2(1e-3));
  return f / max(length(g), 1e-4);
}
float sdRoundBox(vec2 p, vec2 b, float r) {
  vec2 q = abs(p) - b + r;
  return min(max(q.x, q.y), 0.0) + length(max(q, vec2(0.0))) - r;
}
float sdCard(vec2 p, vec2 b, float r, float e, float nd, float nw) {
  float base = sdRoundBox(p, b, r) * 0.6 + sdSquircle(p, b, e) * 0.4;
  if (nd <= 0.0) return base;
  vec2 q = p - vec2(b.x - nw * 0.5, b.y);
  return min(base, max(length(q) - nw * 0.5, -nd));
}
float smin(float a, float b, float k) {
  float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
  return mix(b, a, h) - k * h * (1.0 - h);
}
vec3 rgb2hsv(vec3 c) {
  vec4 K = vec4(0.0, -1.0/3.0, 2.0/3.0, -1.0);
  vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
  vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
  float d = q.x - min(q.w, q.y);
  return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + 1e-10)), d / (q.x + 1e-10), q.x);
}
vec3 hsv2rgb(vec3 c) {
  vec3 p = abs(fract(c.xxx + vec3(0.0, 2.0/3.0, 1.0/3.0)) * 6.0 - 3.0);
  return c.z * mix(vec3(1.0), clamp(p - 1.0, 0.0, 1.0), c.y);
}
