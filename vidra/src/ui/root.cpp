#include "vidra/ui/root.hpp"
#include <algorithm>

namespace vidra::ui {

void Root::setChild(std::unique_ptr<Widget> w) {
  root_ = std::move(w);
  focused_ = nullptr;
  pressed_ = nullptr;
  hovered_ = nullptr;
}
void Root::update(double dt, bool reduce) { if (root_) root_->update(dt, reduce); }
void Root::paint(Painter& p, const Palette& pal) { if (root_) root_->paint(p, pal); }

void Root::collect(Widget* w, std::vector<Widget*>& out) {
  if (!w || !w->visible) return;
  if (w->focusable && !w->disabled) out.push_back(w);
  for (auto& k : w->kids) collect(k.get(), out);
}

void Root::focusNext() {
  std::vector<Widget*> all;
  collect(root_.get(), all);
  if (all.empty()) return;
  auto it = std::find(all.begin(), all.end(), focused_);
  Widget* next = (it == all.end() || it + 1 == all.end()) ? all.front() : *(it + 1);
  if (focused_ && focused_ != next) focused_->onFocus(false);
  focused_ = next;
  focused_->onFocus(true);
}

bool Root::pointer(float x, float y, bool pressed, int button) {
  if (!root_) return false;
  if (pressed) {
    Widget* hit = root_->hit(x, y);
    if (hit != hovered_) {
      if (hovered_) hovered_->onHover(false);
      hovered_ = hit;
      if (hit) hit->onHover(true);
    }
    if (!hit) {
      if (focused_) { focused_->onFocus(false); focused_ = nullptr; }
      return false;
    }
    if (focused_ && focused_ != hit && hit->focusable) { focused_->onFocus(false); focused_ = hit; hit->onFocus(true); }
    pressed_ = hit;
    hit->onPointer(x, y, true, button);
    return true;
  }
  bool consumed = false;
  if (pressed_) { consumed = pressed_->onPointer(x, y, false, button); pressed_ = nullptr; }
  Widget* hit = root_->hit(x, y);
  if (hit != hovered_) {
    if (hovered_) hovered_->onHover(false);
    hovered_ = hit;
    if (hit) hit->onHover(true);
  }
  return consumed;
}

bool Root::key(const Key& k, bool pressed) {
  if (k.code == 0xFF09 && pressed) { focusNext(); return true; }
  if (focused_) return focused_->onKey(k, pressed);
  return false;
}

} // namespace vidra::ui
