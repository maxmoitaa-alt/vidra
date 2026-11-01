#pragma once
#include "vidra/color/palette.hpp"
#include "vidra/gfx/renderer.hpp"
#include "vidra/gfx/text.hpp"
#include "vidra/motion/spring.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace vidra::ui {

using color::Palette;
using color::Role;
using gfx::RectF;

struct Key {
  int code = 0;
  bool shift = false, ctrl = false, alt = false;
  std::string text;
};

class Painter {
 public:
  Painter(gfx::Renderer& r, gfx::TextEngine& t) : r_(r), t_(t) {}
  void rect(RectF b, float radius, color::Rgba8 c, float alpha = 1.0f);
  void card(RectF b, float radius, float notch, color::Rgba8 c);
  void glass(RectF b, float radius, float thickness, float hover);
  void shadow(RectF b, float radius, float blur, float alpha, color::Rgba8 tint);
  void text(const std::string& s, float x, float y, float px, color::Rgba8 c);
  void glyph(const std::string& name, RectF b, color::Rgba8 c);
  float measure(const std::string& s, float px);
  gfx::TextEngine& engine() { return t_; }
  gfx::Renderer& renderer() { return r_; }

 private:
  gfx::Renderer& r_;
  gfx::TextEngine& t_;
};

enum class State { Normal, Hover, Press, Drag };

class Widget {
 public:
  virtual ~Widget() = default;
  RectF bounds;
  bool visible = true;
  bool focusable = false;
  bool disabled = false;
  bool selected = false;
  bool focused = false;
  State state = State::Normal;
  Widget* parent = nullptr;
  std::vector<std::unique_ptr<Widget>> kids;
  motion::Spring radius{380, 0.78f, 1.0f};
  motion::Spring lift{380, 0.78f, 1.0f};

  virtual void layout(RectF b);
  virtual void update(double dt, bool reduce);
  virtual void paint(Painter& p, const Palette& pal);
  virtual bool onPointer(float x, float y, bool pressed, int button);
  virtual bool onKey(const Key& k, bool pressed);
  virtual void onHover(bool in);
  virtual void onFocus(bool on);
  virtual bool consumesDrag() const { return false; }

  void add(std::unique_ptr<Widget> w);
  Widget* hit(float x, float y);
  void clearHover();
  void requestFocus();
  RectF absBounds() const;
  float stateAlpha() const;

 protected:
  void paintBase(Painter& p, const Palette& pal, Role fill, float baseRadius);
  void paintFocus(Painter& p, const Palette& pal);
};

} // namespace vidra::ui
