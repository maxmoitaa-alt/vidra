#include "vidra/gfx/sdf.hpp"
#include <algorithm>

namespace vidra::gfx {
float sdSquircle(Pt p, Pt b, float n) {
  const float sx = std::fabs(p.x) / std::max(b.x, 1e-3f), sy = std::fabs(p.y) / std::max(b.y, 1e-3f);
  const float f = std::pow(sx, n) + std::pow(sy, n) - 1.0f;
  const float gx = n * std::pow(std::max(sx, 1e-4f), n - 1.0f) / std::max(b.x, 1e-3f);
  const float gy = n * std::pow(std::max(sy, 1e-4f), n - 1.0f) / std::max(b.y, 1e-3f);
  return f / std::max(std::sqrt(gx * gx + gy * gy), 1e-4f);
}
float sdRoundBox(Pt p, Pt b, float r) {
  const Pt q{std::fabs(p.x) - b.x + r, std::fabs(p.y) - b.y + r};
  return std::min(std::max(q.x, q.y), 0.0f) + std::hypot(std::max(q.x, 0.0f), std::max(q.y, 0.0f)) - r;
}
float smoothMin(float a, float b, float k) {
  const float h = std::clamp(0.5f + 0.5f * (b - a) / k, 0.0f, 1.0f);
  return b + (a - b) * h - k * h * (1.0f - h);
}
float sdCard(Pt p, Pt b, float r, float e, float nd, float nw) {
  const float base = sdRoundBox(p, b, r) * 0.6f + sdSquircle(p, b, e) * 0.4f;
  if (nd <= 0.0f) return base;
  const Pt q{p.x - (b.x - nw * 0.5f), p.y - b.y};
  return std::min(base, std::max(std::hypot(q.x, q.y) - nw * 0.5f, -nd));
}
bool inside(const RectF& r, float radius, float e, float px, float py) {
  const Pt half{r.w * 0.5f, r.h * 0.5f};
  return sdCard({px - (r.x + half.x), py - (r.y + half.y)}, half, radius, e, 0.0f, 0.0f) <= 0.0f;
}
void inputRegion(const RectF& r, float radius, int step,
                 void (*emit)(int, int, int, int, void*), void* ud) {
  if (step <= 0 || !emit || r.w <= 0 || r.h <= 0) return;
  const int rows = int(std::ceil(r.h / step));
  const float half = r.h * 0.5f;
  for (int i = 0; i < rows; ++i) {
    const int y = int(r.y) + i * step;
    const float dy = (y + step * 0.5f) - (r.y + half);
    const float inset = (radius > 0.5f && std::fabs(dy) > half - radius)
      ? radius - std::sqrt(std::max(radius * radius - std::pow(std::fabs(dy) - (half - radius), 2.0f), 0.0f))
      : 0.0f;
    const int x = int(r.x + inset), w = int(r.w - inset * 2.0f);
    const int hh = std::min(step, int(r.y + r.h) - y);
    if (w > 0 && hh > 0) emit(x, y, w, hh, ud);
  }
}
} // namespace vidra::gfx
