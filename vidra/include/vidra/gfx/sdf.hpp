#pragma once
#include <cmath>

namespace vidra::gfx {
struct RectF { float x, y, w, h; };
struct Pt { float x, y; };
float sdSquircle(Pt p, Pt half, float e);
float sdRoundBox(Pt p, Pt half, float r);
float sdCard(Pt p, Pt half, float r, float e, float nd, float nw);
float smoothMin(float a, float b, float k);
bool inside(const RectF& r, float radius, float e, float px, float py);
void inputRegion(const RectF& r, float radius, int step,
                 void (*emit)(int, int, int, int, void*), void* ud);
} // namespace vidra::gfx
