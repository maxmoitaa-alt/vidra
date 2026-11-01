#include "vidra/ui/widget.hpp"
#include "vidra/gfx/sdf.hpp"
#include <algorithm>

namespace vidra::ui {

void Painter::rect(RectF b, float radius, color::Rgba8 c, float alpha) {
  gfx::Quad q{};
  q.mat = gfx::Material::Tonal;
  q.box = b;
  q.radius = radius;
  q.exponent = tok::kSquircleExponent;
  q.color = c;
  q.alpha = alpha;
  r_.push(q);
}
void Painter::card(RectF b, float radius, float notch, color::Rgba8 c) {
  gfx::Quad q{};
  q.mat = gfx::Material::Tonal;
  q.box = b;
  q.radius = radius;
  q.exponent = tok::kSquircleExponent;
  q.notchDepth = notch;
  q.color = c;
  r_.push(q);
}
void Painter::glass(RectF b, float radius, float thickness, float hover) {
  gfx::Quad q{};
  q.mat = gfx::Material::Glass;
  q.box = b;
  q.radius = radius;
  q.exponent = tok::kSquircleExponent;
  q.thickness = thickness;
  q.alpha = hover;
  q.color = {255, 255, 255, 255};
  r_.push(q);
}
void Painter::shadow(RectF b, float radius, float blur, float alpha, color::Rgba8 tint) {
  gfx::Quad q{};
  q.mat = gfx::Material::Shadow;
  q.box = b;
  q.radius = radius;
  q.blur = blur;
  q.alpha = alpha;
  q.color = tint;
  r_.push(q);
}
void Painter::text(const std::string& s, float x, float y, float px, color::Rgba8 c) {
  std::vector<float> adv;
  const auto slots = t_.layout(s, px, &adv);
  float pen = x;
  for (size_t i = 0; i < slots.size(); ++i) {
    const auto& sl = slots[i];
    if (sl.w > 0) {
      gfx::Quad q{};
      q.mat = gfx::Material::Text;
      q.box = {pen, y - float(sl.bearing), float(sl.w), float(sl.h)};
      q.uv = {sl.u0, sl.v0, sl.u1 - sl.u0, sl.v1 - sl.v0};
      q.color = c;
      r_.push(q);
    }
    if (i < adv.size()) pen += adv[i];
  }
}
void Painter::glyph(const std::string& name, RectF b, color::Rgba8 c) {
  text(name, b.x, b.y + b.h * 0.8f, b.h, c);
}
float Painter::measure(const std::string& s, float px) { return t_.measure(s, px); }

void Widget::layout(RectF b) {
  bounds = b;
  for (auto& k : kids) k->layout(b);
}
void Widget::update(double dt, bool reduce) {
  radius.step(dt, reduce);
  lift.step(dt, reduce);
  for (auto& k : kids) if (k->visible) k->update(dt, reduce);
}
void Widget::paint(Painter& p, const Palette& pal) {
  if (!visible) return;
  paintBase(p, pal, Role::SurfaceContainer, 12.0f);
  paintFocus(p, pal);
  for (auto& k : kids) k->paint(p, pal);
}
float Widget::stateAlpha() const {
  switch (state) {
    case State::Hover: return tok::kStateHover;
    case State::Press: return tok::kStatePress;
    case State::Drag: return tok::kStateDrag;
    default: return 0.0f;
  }
}
void Widget::paintBase(Painter& p, const Palette& pal, Role fill, float baseRadius) {
  const float r = radius.x > 0.5f ? radius.x : baseRadius;
  const RectF b{bounds.x, bounds.y + lift.x, bounds.w, bounds.h};
  p.rect(b, r, pal[disabled ? Role::SurfaceContainerHigh : fill]);
  const float a = stateAlpha();
  if (a > 0.001f) p.rect(b, r, pal[Role::OnSurface], a * 0.12f);
}
void Widget::paintFocus(Painter& p, const Palette& pal) {
  if (!focused) return;
  p.rect({bounds.x - 3, bounds.y - 3, bounds.w + 6, bounds.h + 6}, 16.0f, pal[Role::Secondary], 0.9f);
  p.rect({bounds.x - 1, bounds.y - 1, bounds.w + 2, bounds.h + 2}, 14.0f, pal[Role::Surface]);
}
bool Widget::onPointer(float, float, bool pressed, int) {
  if (disabled) return false;
  if (pressed) { state = State::Press; radius.to(20.0f); }
  else { state = State::Normal; radius.to(12.0f); }
  return true;
}
bool Widget::onKey(const Key&, bool) { return false; }
void Widget::onHover(bool in) {
  if (disabled) return;
  if (in) { state = State::Hover; lift.to(-tok::kElevHoverLift); }
  else { state = State::Normal; lift.to(0.0f); }
}
void Widget::onFocus(bool on) { focused = on; }
void Widget::add(std::unique_ptr<Widget> w) { w->parent = this; kids.push_back(std::move(w)); }
Widget* Widget::hit(float x, float y) {
  if (!visible || disabled) return nullptr;
  for (auto it = kids.rbegin(); it != kids.rend(); ++it) {
    if (auto* h = (*it)->hit(x, y)) return h;
  }
  return gfx::inside(bounds, radius.x > 0.5f ? radius.x : 12.0f, tok::kSquircleExponent, x, y)
           ? this : nullptr;
}
void Widget::clearHover() {
  if (state == State::Hover) onHover(false);
  for (auto& k : kids) k->clearHover();
}
void Widget::requestFocus() { focused = true; }
RectF Widget::absBounds() const {
  RectF r = bounds;
  for (const Widget* p = parent; p; p = p->parent) { r.x += p->bounds.x; r.y += p->bounds.y; }
  return r;
}

} // namespace vidra::ui
