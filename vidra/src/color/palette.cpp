#include "vidra/color/palette.hpp"
#include <algorithm>
#include <cstdlib>

namespace vidra::color {
namespace {
Rgba8 tone(double h, double c, double t) { return rgbaFromHct({h, c, t}); }
} // namespace

uint32_t parseHex(const std::string& s) {
  const char* p = s.c_str();
  if (*p == '#') ++p;
  return uint32_t(std::strtoul(p, nullptr, 16)) & 0xFFFFFF;
}

Palette buildPalette(uint32_t seed, bool dark) {
  const Hct sh = hctFromRgba({uint8_t(seed >> 16), uint8_t(seed >> 8), uint8_t(seed), 255});
  const double hp = sh.hue;
  const double hs = std::fmod(hp + 12.0, 360.0);
  const double ht = std::fmod(hp + tok::kHueOffsetTertiary, 360.0);
  const double cp = std::max<double>(tok::kChromaPrimary, 40.0);
  const double cs = std::max<double>(tok::kChromaSecondary, 16.0);
  const double ct = std::max<double>(tok::kChromaTertiary, 32.0);
  const double cn = tok::kChromaNeutral, cnv = tok::kChromaNeutralVariant;
  Palette p{};
  auto pair = [&](Role fg, Role bg, double hue, double ch, double tf, double tb) {
    p[fg] = tone(hue, ch * 0.4, dark ? 100.0 - tf : tf);
    p[bg] = tone(hue, ch, dark ? 100.0 - tb : tb);
  };
  pair(Role::OnPrimary, Role::Primary, hp, cp, dark ? 20 : 100, dark ? 80 : 40);
  pair(Role::OnPrimaryContainer, Role::PrimaryContainer, hp, cp, dark ? 90 : 10, dark ? 30 : 90);
  pair(Role::OnSecondary, Role::Secondary, hs, cs, dark ? 20 : 100, dark ? 80 : 40);
  pair(Role::OnSecondaryContainer, Role::SecondaryContainer, hs, cs, dark ? 90 : 10, dark ? 30 : 90);
  pair(Role::OnTertiary, Role::Tertiary, ht, ct, dark ? 20 : 100, dark ? 80 : 40);
  pair(Role::OnTertiaryContainer, Role::TertiaryContainer, ht, ct, dark ? 90 : 10, dark ? 30 : 90);
  pair(Role::OnError, Role::Error, 25.0, 72.0, dark ? 20 : 100, dark ? 80 : 40);
  pair(Role::OnErrorContainer, Role::ErrorContainer, 25.0, 72.0, dark ? 90 : 10, dark ? 30 : 90);
  p[Role::Surface] = tone(hp, cn, dark ? 6 : 98);
  p[Role::OnSurface] = tone(hp, cn, dark ? 90 : 10);
  p[Role::SurfaceVariant] = tone(hp, cnv, dark ? 30 : 90);
  p[Role::OnSurfaceVariant] = tone(hp, cnv, dark ? 80 : 30);
  p[Role::SurfaceLowest] = tone(hp, cn, dark ? 4 : 100);
  p[Role::SurfaceLow] = tone(hp, cn, dark ? 10 : 96);
  p[Role::SurfaceContainer] = tone(hp, cn, dark ? 12 : 94);
  p[Role::SurfaceHigh] = tone(hp, cn, dark ? 17 : 92);
  p[Role::SurfaceHighest] = tone(hp, cn, dark ? 22 : 90);
  p[Role::Outline] = tone(hp, cnv, dark ? 60 : 50);
  p[Role::OutlineVariant] = tone(hp, cnv, dark ? 30 : 80);
  p[Role::InverseSurface] = tone(hp, cn, dark ? 90 : 20);
  p[Role::InverseOnSurface] = tone(hp, cn, dark ? 20 : 95);
  p[Role::InversePrimary] = tone(hp, cp, dark ? 40 : 80);
  p[Role::SurfaceTint] = p[Role::Primary];
  const std::array<std::pair<Role, double>, 5> tinted = {{
    {Role::Surface, tok::kSurfaceTintMixLow}, {Role::SurfaceLowest, tok::kSurfaceTintMixLow},
    {Role::SurfaceLow, tok::kSurfaceTintMixLow * 1.4},
    {Role::SurfaceContainer, tok::kSurfaceTintMixHigh * 0.7},
    {Role::SurfaceHigh, tok::kSurfaceTintMixHigh}}};
  for (auto [r, m] : tinted) p[r] = mixRgba(p[r], p[Role::SurfaceTint], std::min(m, static_cast<double>(tok::kSurfaceTintMixHigh)));
  enforceContrast(p);
  return p;
}

Palette blendPalette(const Palette& a, const Palette& b, double t) {
  Palette o{};
  for (size_t i = 0; i < static_cast<size_t>(Role::Count); ++i) o.v[i] = mixRgba(a.v[i], b.v[i], t);
  return o;
}

void enforceContrast(Palette& p) {
  const std::array<std::pair<Role, Role>, 8> pairs = {{
    {Role::OnPrimary, Role::Primary}, {Role::OnPrimaryContainer, Role::PrimaryContainer},
    {Role::OnSecondary, Role::Secondary}, {Role::OnTertiary, Role::Tertiary},
    {Role::OnSurface, Role::Surface}, {Role::OnSurface, Role::SurfaceHigh},
    {Role::OnErrorContainer, Role::ErrorContainer},
    {Role::OnSurfaceVariant, Role::SurfaceVariant}}};
  for (auto [fg, bg] : pairs) p[fg] = ensureContrast(p[fg], p[bg], tok::kContrastMin);
}
} // namespace vidra::color
