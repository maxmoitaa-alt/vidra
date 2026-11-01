#pragma once
#include "vidra/tokens.hpp"
#include <algorithm>
#include <array>
#include <cmath>

namespace vidra::motion {
struct Spring {
  float stiffness = tok::kSpringInStiffness, damping = tok::kSpringInDamping, mass = tok::kSpringInMass;
  float x = 0, v = 0, target = 0;
  bool settled = true;
  void to(float t) { target = t; settled = false; }
  void snap(float t) { x = target = t; v = 0; settled = true; }
  bool step(double dt, bool reduce = false) {
    if (reduce) { snap(target); return false; }
    constexpr float kSub = 1.0f / 600.0f;
    double left = std::min(dt, 0.05);
    while (left > 0.0) {
      const float h = float(std::min(left, double(kSub)));
      v += (-stiffness * (x - target) - damping * v) / std::max(mass, 1e-3f) * h;
      x += v * h;
      left -= h;
    }
    if (std::fabs(v) < 0.6f && std::fabs(x - target) < 0.25f) { x = target; v = 0; settled = true; }
    return !settled;
  }
};
inline float bezier(const std::array<float, 4>& c, float t) {
  auto bx = [&](float u) { const float i = 1 - u; return 3*i*i*u*c[0] + 3*i*u*u*c[2] + u*u*u; };
  auto by = [&](float u) { const float i = 1 - u; return 3*i*i*u*c[1] + 3*i*u*u*c[3] + u*u*u; };
  float u = t;
  for (int k = 0; k < 6; ++k) {
    const float x = bx(u) - t;
    const float d = 3*(1-u)*(1-u)*c[0] + 6*(1-u)*u*(c[2]-c[0]) + 3*u*u*(1-c[2]);
    if (std::fabs(d) < 1e-5f) break;
    u = std::clamp(u - x / d, 0.0f, 1.0f);
  }
  return by(u);
}
} // namespace vidra::motion
