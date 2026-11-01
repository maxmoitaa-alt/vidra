#include "vidra/color/hct.hpp"
#include <algorithm>

namespace vidra::color {
namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kM[3][3] = {{0.41233895, 0.35762064, 0.18051042},
                             {0.21263901, 0.71516868, 0.07219232},
                             {0.01933082, 0.11919478, 0.95053215}};
constexpr double kI[3][3] = {{3.24137748, -1.53713885, -0.49853155},
                             {-0.96926639, 1.87601080, 0.04155608},
                             {0.05564342, -0.20402585, 1.05722516}};
constexpr double kW[3] = {0.95045592705167, 1.0, 1.08905775075988};
struct Vc { double n, aw, nbb, ncb, c, nc, d[3], fl, z; };
const Vc& vc() {
  static const Vc v = [] {
    constexpr double la = 63.66197723675813;
    const double dd = 0.9 * (1.0 - (1.0 / 3.6) * 0.5382) + 0.1;
    Vc o{};
    o.n = 18.0 / kW[1]; o.z = 1.48 + std::sqrt(o.n);
    o.nbb = 0.725 / std::pow(o.n, 0.2); o.ncb = o.nbb; o.c = 0.69; o.nc = 1.0;
    o.fl = 0.2 * std::pow(la, 1.0 / 3.0) * 0.005 + 0.1;
    o.d[0] = dd * kW[0] / 0.4106 + 1.0 - dd;
    o.d[1] = dd * kW[1] / 0.6873 + 1.0 - dd;
    o.d[2] = dd * kW[2] / 0.1653 + 1.0 - dd;
    o.aw = (40.0 * kW[0] * o.d[0] + 20.0 * kW[1] * o.d[1] + kW[2] * o.d[2]) * o.nbb;
    return o;
  }();
  return v;
}
double adapt(double c, double fl) {
  const double a = std::pow(fl * std::fabs(c) / 100.0, 0.42);
  return std::copysign(400.0 * a / (a + 27.13), c);
}
double unadapt(double a, double fl) {
  const double b = std::max(0.0, 27.13 * std::fabs(a) / (400.0 - std::fabs(a)));
  return std::copysign(100.0 / fl * std::pow(b, 1.0 / 0.42), a);
}
} // namespace

double srgbToLinear(double c) { return c <= 0.04045 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4); }
double linearToSrgb(double c) {
  c = std::clamp(c, 0.0, 1.0);
  return c <= 0.0031308 ? c * 12.92 : 1.055 * std::pow(c, 1.0 / 2.4) - 0.055;
}
Rgba8 encodeSrgb(const Rgb& l, uint8_t a) {
  auto q = [](double v) { return uint8_t(std::lround(std::clamp(linearToSrgb(v), 0.0, 1.0) * 255.0)); };
  return {q(l.r), q(l.g), q(l.b), a};
}
Xyz linToXyz(const Rgb& v) {
  return {kM[0][0]*v.r + kM[0][1]*v.g + kM[0][2]*v.b,
          kM[1][0]*v.r + kM[1][1]*v.g + kM[1][2]*v.b,
          kM[2][0]*v.r + kM[2][1]*v.g + kM[2][2]*v.b};
}
Rgb xyzToLin(const Xyz& v) {
  return {kI[0][0]*v.x + kI[0][1]*v.y + kI[0][2]*v.z,
          kI[1][0]*v.x + kI[1][1]*v.y + kI[1][2]*v.z,
          kI[2][0]*v.x + kI[2][1]*v.y + kI[2][2]*v.z};
}
bool inGamut(const Xyz& v) {
  const Rgb l = xyzToLin(v);
  constexpr double e = 1e-4;
  return l.r >= -e && l.r <= 1 + e && l.g >= -e && l.g <= 1 + e && l.b >= -e && l.b <= 1 + e;
}
double yToTone(double y) {
  constexpr double e = 216.0 / 24389.0, k = 24389.0 / 27.0;
  const double r = std::clamp(y / 100.0, 0.0, 1.0);
  return r > e ? std::cbrt(r) * 116.0 - 16.0 : r * k;
}
double toneToY(double t) {
  constexpr double e = 216.0 / 24389.0, k = 24389.0 / 27.0;
  const double c = std::clamp(t, 0.0, 100.0);
  return (c < 8.0 ? c / k : std::pow((c + 16.0) / 116.0, 3.0)) * 100.0;
}
double luminance(const Rgba8& c) {
  return 0.2126 * srgbToLinear(c.r / 255.0) + 0.7152 * srgbToLinear(c.g / 255.0) +
         0.0722 * srgbToLinear(c.b / 255.0);
}
double contrastRatio(const Rgba8& a, const Rgba8& b) {
  const double x = luminance(a) + 0.05, y = luminance(b) + 0.05;
  return x > y ? x / y : y / x;
}
Rgba8 mixRgba(const Rgba8& a, const Rgba8& b, double t) {
  auto f = [t](uint8_t x, uint8_t y) { return uint8_t(std::lround(x + (y - x) * t)); };
  return {f(a.r, b.r), f(a.g, b.g), f(a.b, b.b), f(a.a, b.a)};
}
Cam16 Cam16::fromXyz(const Xyz& v) {
  const Vc& w = vc();
  const double r = v.x * w.d[0] * 0.401288 + v.y * w.d[1] * 0.650173 + v.z * w.d[2] * -0.051461;
  const double g = v.x * w.d[0] * -0.250268 + v.y * w.d[1] * 1.204414 + v.z * w.d[2] * 0.045854;
  const double b = v.x * w.d[0] * -0.002079 + v.y * w.d[1] * 0.048952 + v.z * w.d[2] * 0.953127;
  const double ra = adapt(r, w.fl), ga = adapt(g, w.fl), ba = adapt(b, w.fl);
  const double a = (11.0 * ra - 12.0 * ga + ba) / 11.0;
  const double bb = (ra + ga - 2.0 * ba) / 9.0;
  const double u = (20.0 * ra + 20.0 * ga + 21.0 * ba) / 20.0;
  const double p2 = (40.0 * ra + 20.0 * ga + ba) / 20.0;
  double h = std::atan2(bb, a) * 180.0 / kPi;
  if (h < 0) h += 360.0;
  const double j = 100.0 * std::pow(std::max(p2 * w.nbb, 0.0) / w.aw, w.c * w.z);
  const double hr = h * kPi / 180.0;
  const double eh = 0.25 * (std::cos(hr + 2.0) + 3.8);
  const double t = (50000.0 / 13.0) * eh * w.nc * w.ncb * std::hypot(a, bb) / (u + 0.305);
  return {j, std::pow(t, 0.9) * std::pow(1.64 - std::pow(0.29, w.n), 0.73), h};
}
Xyz Cam16::toXyz() const {
  const Vc& w = vc();
  const double a = std::max(c, 0.0);
  const double hr = h * kPi / 180.0;
  const double t = std::pow(a / std::pow(1.64 - std::pow(0.29, w.n), 0.73), 1.0 / 0.9);
  const double eh = 0.25 * (std::cos(hr + 2.0) + 3.8);
  const double ac = w.aw * std::pow(std::max(j, 0.0) / 100.0, 1.0 / w.c / w.z);
  const double p1 = eh * (50000.0 / 13.0) * w.nc * w.ncb;
  const double p2 = ac / w.nbb;
  const double hs = std::sin(hr), hc = std::cos(hr);
  const double gg = 23.0 * (p2 + 0.305) * t / (23.0 * p1 + 11.0 * t * hc + 108.0 * t * hs);
  const double aa = gg * hc, bb = gg * hs;
  const double rA = (460.0 * p2 + 451.0 * aa + 288.0 * bb) / 1403.0;
  const double gA = (460.0 * p2 - 891.0 * aa - 261.0 * bb) / 1403.0;
  const double bA = (460.0 * p2 - 220.0 * aa - 6300.0 * bb) / 1403.0;
  const double rF = unadapt(rA, w.fl) / w.d[0];
  const double gF = unadapt(gA, w.fl) / w.d[1];
  const double bF = unadapt(bA, w.fl) / w.d[2];
  return {rF * 1.8620678 + gF * -1.0112547 + bF * 0.14918678,
          rF * -0.3873268 + gF * 2.0465678 + bF * -0.5598131,
          rF * 0.0665045 + gF * -0.2288873 + bF * 1.1623633};
}
Hct hctFromRgba(const Rgba8& c) {
  const Xyz x = linToXyz({srgbToLinear(c.r / 255.0), srgbToLinear(c.g / 255.0),
                          srgbToLinear(c.b / 255.0)});
  const Cam16 cam = Cam16::fromXyz(x);
  return {cam.h, cam.c, yToTone(x.y)};
}
Rgba8 rgbaFromHct(const Hct& h) {
  const double ty = toneToY(std::clamp(h.tone, 0.0, 100.0));
  auto solve = [&](double ch) {
    double lo = 0.0, hi = 100.0;
    Xyz best{};
    for (int i = 0; i < 26; ++i) {
      const double jm = 0.5 * (lo + hi);
      Xyz v = Cam16{jm, ch, h.hue}.toXyz();
      const double s = ty / std::max(v.y, 1e-6);
      v.x *= s; v.y = ty; v.z *= s;
      best = v;
      if (Cam16::fromXyz(v).j < jm) lo = jm; else hi = jm;
    }
    return best;
  };
  double ch = h.chroma;
  Xyz v = solve(ch);
  for (int i = 0; i < 20 && !inGamut(v); ++i) { ch *= 0.92; v = solve(ch); }
  const Rgb l = xyzToLin(v);
  auto cl = [](double x) { return std::clamp(x, 0.0, 1.0); };
  return encodeSrgb({cl(l.r), cl(l.g), cl(l.b)});
}
Rgba8 ensureContrast(Rgba8 fg, const Rgba8& bg, double min) {
  if (contrastRatio(fg, bg) >= min) return fg;
  const bool light = luminance(bg) > 0.4;
  Hct h = hctFromRgba(fg);
  for (int i = 0; i < 40; ++i) {
    h.tone = std::clamp(h.tone + (light ? -2.0 : 2.0), 0.0, 100.0);
    fg = rgbaFromHct(h);
    if (contrastRatio(fg, bg) >= min) break;
  }
  return fg;
}
} // namespace vidra::color
