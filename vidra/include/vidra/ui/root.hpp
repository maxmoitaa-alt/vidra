#pragma once
#include "vidra/ui/widget.hpp"

namespace vidra::ui {

class Root {
 public:
  void setChild(std::unique_ptr<Widget> w);
  void update(double dt, bool reduce);
  void paint(Painter& p, const Palette& pal);
  bool pointer(float x, float y, bool pressed, int button);
  bool key(const Key& k, bool pressed);
  void focusNext();
  Widget* focused() const { return focused_; }
  Widget* child() const { return root_.get(); }

 private:
  std::unique_ptr<Widget> root_;
  Widget* focused_ = nullptr;
  Widget* pressed_ = nullptr;
  Widget* hovered_ = nullptr;
  void collect(Widget* w, std::vector<Widget*>& out);
};

} // namespace vidra::ui
