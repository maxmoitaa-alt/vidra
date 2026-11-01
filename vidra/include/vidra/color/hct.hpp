#pragma once
#include <cmath>
#include <cstdint>
#include <string>

namespace vidra::color {

struct Rgb { double r, g, b; };
struct Rgba8 { uint8_t r, g, b, a; };

double srgbToLinear(double c);
double linearToSrgb(double c);
Rgba8 encodeSrgb(const Rgb& l, uint8_t a = 255);

struct Xyz { double x, y, z; };
Xyz linToXyz(const Rgb& l);
Rgb xyzToLin(const Xyz& v);
bool inGamut(const Xyz& v);
double yToTone(double y);
double toneToY(double t);
double luminance(const Rgba8& c);
double contrastRatio(const Rgba8& a, const Rgba8& b);
Rgba8 mixRgba(const Rgba8& a, const Rgba8& b, double t);

struct Cam16 { double j, c, h; static Cam16 fromXyz(const Xyz&); Xyz toXyz() const; };
struct Hct { double hue, chroma, tone; };

Hct hctFromRgba(const Rgba8& c);
Rgba8 rgbaFromHct(const Hct& h);
Rgba8 ensureContrast(Rgba8 fg, const Rgba8& bg, double minRatio);

} // namespace vidra::color
