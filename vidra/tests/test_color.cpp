#include "check.hpp"
#include "vidra/color/palette.hpp"
int main() {
  using namespace vidra::color;
  const Palette d = buildPalette(0x6750A4, true);
  CHECK(contrastRatio(d[Role::OnPrimary], d[Role::Primary]) >= 4.5);
  CHECK(contrastRatio(d[Role::OnSurface], d[Role::Surface]) >= 4.5);
  const Palette l = buildPalette(0x6750A4, false);
  CHECK(contrastRatio(l[Role::OnSurface], l[Role::Surface]) >= 4.5);
  CHECK(parseHex("#8E4585") == 0x8E4585);
  return report("color");
}
