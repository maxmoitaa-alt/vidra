#pragma once
#include "vidra/color/hct.hpp"
#include "vidra/tokens.hpp"
#include <array>
#include <string>

namespace vidra::color {
enum class Role : int {
  Primary, OnPrimary, PrimaryContainer, OnPrimaryContainer,
  Secondary, OnSecondary, SecondaryContainer, OnSecondaryContainer,
  Tertiary, OnTertiary, TertiaryContainer, OnTertiaryContainer,
  Error, OnError, ErrorContainer, OnErrorContainer,
  Surface, OnSurface, SurfaceVariant, OnSurfaceVariant,
  SurfaceLowest, SurfaceLow, SurfaceContainer, SurfaceHigh, SurfaceHighest,
  SurfaceTint, Outline, OutlineVariant, InverseSurface, InverseOnSurface, InversePrimary,
  Count
};
struct Palette {
  std::array<Rgba8, static_cast<size_t>(Role::Count)> v{};
  Rgba8& operator[](Role r) { return v[static_cast<size_t>(r)]; }
  const Rgba8& operator[](Role r) const { return v[static_cast<size_t>(r)]; }
};
Palette buildPalette(uint32_t seed, bool dark);
Palette blendPalette(const Palette& a, const Palette& b, double t);
void enforceContrast(Palette& p);
uint32_t parseHex(const std::string& s);
} // namespace vidra::color
