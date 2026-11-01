#include "vidra/ui/widgets.hpp"
#include <algorithm>
#include <cmath>

namespace vidra::ui {

Button::Button(std::string l) : label_(std::move(l)) {
  focusable = true;
  bounds = {0, 0, 140, 48};
}
void Button::layout(RectF b) { bounds = b; }
void Button::paint(Painter& p, const Palette& pal) {
  if (!visible) return;
  const bool primary = selected;
  paintBase(p, pal, primary ? Role::Primary : Role::SurfaceContainerHigh, 24.0f);
  const float w = p.measure(label_, tok::kTypeLabelLargeSize);
  p.text(label_, bounds.x + (bounds.w - w) * 0.5f, bounds.y + bounds.h * 0.5f + 5.0f,
         tok::kTypeLabelLargeSize, pal[primary ? Role::OnPrimary : Role::OnSurface]);
  paintFocus(p, pal);
}
bool Button::onPointer(float, float, bool pressed, int) {
  if (disabled) return false;
  if (pressed) { state = State::Press; return true; }
  state = State::Hover;
  if (onClick) onClick();
  return true;
}

IconButton::IconButton(std::string g, std::string t) : glyph_(std::move(g)), tip_(std::move(t)) {
  focusable = true;
  bounds = {0, 0, 48, 48};
}
void IconButton::paint(Painter& p, const Palette& pal) {
  if (!visible) return;
  paintBase(p, pal, Role::SurfaceContainerHigh, 24.0f);
  p.glyph(glyph_, {bounds.x + 12, bounds.y + 12, 24, 24}, pal[Role::OnSurfaceVariant]);
  paintFocus(p, pal);
}
bool IconButton::onPointer(float, float, bool pressed, int) {
  if (disabled) return false;
  if (pressed) { state = State::Press; return true; }
  state = State::Hover;
  if (onClick) onClick();
  return true;
}

Chip::Chip(std::string l) : label_(std::move(l)) {
  focusable = true;
  bounds = {0, 0, 120, 40};
}
void Chip::layout(RectF b) {
  bounds = b;
  bounds.w = std::clamp(24.0f + float(label_.size()) * 8.0f, 90.0f, 260.0f);
}
void Chip::paint(Painter& p, const Palette& pal) {
  if (!visible) return;
  paintBase(p, pal, selected ? Role::SecondaryContainer : Role::SurfaceContainerHigh,
            selected ? 20.0f : 8.0f);
  p.text(label_, bounds.x + 14.0f, bounds.y + 26.0f, tok::kTypeLabelLargeSize,
         pal[selected ? Role::OnSecondaryContainer : Role::OnSurfaceVariant]);
  paintFocus(p, pal);
}
bool Chip::onPointer(float, float, bool pressed, int) {
  if (disabled) return false;
  if (pressed) { state = State::Press; return true; }
  state = State::Hover;
  selected = !selected;
  if (onToggle) onToggle(selected);
  return true;
}

Toggle::Toggle() {
  focusable = true;
  bounds = {0, 0, 52, 32};
  knob.snap(24.0f);
}
void Toggle::paint(Painter& p, const Palette& pal) {
  p.rect(bounds, 16.0f, pal[selected ? Role::Primary : Role::SurfaceHighest]);
  const float tx = bounds.x + (selected ? bounds.w - 26.0f : 4.0f);
  p.rect({tx, bounds.y + 4, knob.x, 24}, 12.0f, pal[selected ? Role::OnPrimary : Role::Outline]);
  paintFocus(p, pal);
}
bool Toggle::onPointer(float, float, bool pressed, int) {
  if (disabled) return false;
  if (pressed) { state = State::Press; return true; }
  state = State::Hover;
  selected = !selected;
  knob.snap(30.0f);
  knob.to(24.0f);
  if (onChange) onChange(selected);
  return true;
}

Slider::Slider(float mn, float mx, float v) : min_(mn), max_(mx), value_(v) {
  focusable = true;
  bounds = {0, 0, 240, 32};
}
void Slider::paint(Painter& p, const Palette& pal) {
  const float t = std::clamp((value_ - min_) / (max_ - min_), 0.0f, 1.0f);
  p.rect({bounds.x, bounds.y + 12, bounds.w, 8}, 4.0f, pal[Role::SurfaceHighest]);
  p.rect({bounds.x, bounds.y + 12, bounds.w * t, 8}, 4.0f, pal[Role::Primary]);
  p.rect({bounds.x + bounds.w * t - 10, bounds.y + 6, 20, 20}, 10.0f, pal[Role::Primary]);
  paintFocus(p, pal);
}
bool Slider::onPointer(float x, float, bool pressed, int) {
  if (disabled) return false;
  if (pressed) state = State::Drag;
  const float t = std::clamp((x - bounds.x) / bounds.w, 0.0f, 1.0f);
  value_ = min_ + t * (max_ - min_);
  if (onChange) onChange(value_);
  return true;
}

Field::Field(std::string ph) : ph_(std::move(ph)) {
  focusable = true;
  bounds = {0, 0, 300, 48};
}
void Field::setValue(std::string v) { value_ = std::move(v); }
void Field::paint(Painter& p, const Palette& pal) {
  paintBase(p, pal, Role::SurfaceContainerHigh, 14.0f);
  const bool empty = value_.empty();
  p.text(empty ? ph_ : value_, bounds.x + 14, bounds.y + 30, tok::kTypeBodyLargeSize,
         pal[empty ? Role::OnSurfaceVariant : Role::OnSurface]);
  if (focused) {
    const float w = p.measure(value_, tok::kTypeBodyLargeSize);
    p.rect({bounds.x + 15 + w, bounds.y + 14, 2, 20}, 1.0f, pal[Role::Primary]);
  }
  paintFocus(p, pal);
}
bool Field::onKey(const Key& k, bool pressed) {
  if (!pressed) return false;
  if (k.code == 0xFF08 && !value_.empty()) value_.pop_back();
  else if (k.code == 0xFFFF) value_.clear();
  else if (!k.text.empty() && value_.size() < 120) value_.append(k.text);
  else return false;
  if (onInput) onInput(value_);
  return true;
}
bool Field::onPointer(float, float, bool pressed, int) {
  if (pressed) state = State::Press;
  return true;
}

Card::Card(std::string t, std::string a, std::string g)
    : title_(std::move(t)), author_(std::move(a)), glyph_(std::move(g)) {
  focusable = true;
  bounds = {0, 0, 300, 148};
  radius.snap(28.0f);
}
void Card::layout(RectF b) { bounds = {b.x, b.y, b.w, 148}; }
void Card::paint(Painter& p, const Palette& pal) {
  if (!visible) return;
  p.card({bounds.x, bounds.y + lift.x, bounds.w, bounds.h}, 28.0f, tok::kNotchDepth,
         pal[Role::SurfaceContainerHigh]);
  const float a = stateAlpha();
  if (a > 0.001f) p.rect({bounds.x, bounds.y + lift.x, bounds.w, bounds.h}, 28.0f,
                         pal[Role::OnSurface], a * 0.10f);
  p.glyph(glyph_, {bounds.x + 20, bounds.y + 20, 40, 40}, pal[Role::Primary]);
  p.text(title_, bounds.x + 20, bounds.y + 92, tok::kTypeTitleLargeSize, pal[Role::OnSurface]);
  p.text(author_, bounds.x + 20, bounds.y + 118, tok::kTypeLabelLargeSize, pal[Role::OnSurfaceVariant]);
  p.glyph(fav_ ? "star" : "star_border", {bounds.x + bounds.w - 52, bounds.y + 20, 32, 32},
          pal[fav_ ? Role::Primary : Role::OnSurfaceVariant]);
  paintFocus(p, pal);
}
bool Card::onPointer(float x, float y, bool pressed, int) {
  if (disabled) return false;
  if (pressed) { state = State::Press; return true; }
  state = State::Hover;
  if (x > bounds.x + bounds.w - 56 && onFav) { fav_ = !fav_; onFav(); }
  else if (onOpen) onOpen();
  return true;
}

void Snackbar::show(std::string m) { msg_ = std::move(m); life_ = 3.0f; }
void Snackbar::update(double dt, bool reduce) {
  Widget::update(dt, reduce);
  if (life_ > 0) life_ -= float(dt);
}
void Snackbar::paint(Painter& p, const Palette& pal) {
  if (life_ <= 0) return;
  p.glass(bounds, 12.0f, 24.0f, 0.0f);
  p.text(msg_, bounds.x + 20, bounds.y + 30, tok::kTypeBodyLargeSize, pal[Role::InverseOnSurface]);
}

Dialog::Dialog(std::string t, std::string b) : title_(std::move(t)), body_(std::move(b)) {
  focusable = true;
  bounds = {0, 0, 420, 220};
}
void Dialog::paint(Painter& p, const Palette& pal) {
  p.rect({0, 0, 4000, 4000}, 0.0f, pal[Role::InverseSurface], tok::kElevScrimAlpha);
  p.glass(bounds, 28.0f, 36.0f, 0.0f);
  p.text(title_, bounds.x + 28, bounds.y + 60, tok::kTypeHeadlineSmallSize, pal[Role::OnSurface]);
  p.text(body_, bounds.x + 28, bounds.y + 100, tok::kTypeBodyLargeSize, pal[Role::OnSurfaceVariant]);
  p.rect({bounds.x + bounds.w - 200, bounds.y + bounds.h - 72, 80, 44}, 22.0f,
         pal[Role::SurfaceContainerHigh]);
  p.text("Cancelar", bounds.x + bounds.w - 188, bounds.y + bounds.h - 44,
         tok::kTypeLabelLargeSize, pal[Role::OnSurfaceVariant]);
  p.rect({bounds.x + bounds.w - 108, bounds.y + bounds.h - 72, 80, 44}, 22.0f, pal[Role::Primary]);
  p.text("OK", bounds.x + bounds.w - 84, bounds.y + bounds.h - 44, tok::kTypeLabelLargeSize,
         pal[Role::OnPrimary]);
}
bool Dialog::onPointer(float x, float y, bool pressed, int) {
  if (!pressed) return true;
  if (x > bounds.x + bounds.w - 108) { if (onResult) onResult(true); }
  else if (x > bounds.x + bounds.w - 200 && y > bounds.y + bounds.h - 72) {
    if (onResult) onResult(false);
  }
  return true;
}

} // namespace vidra::ui
