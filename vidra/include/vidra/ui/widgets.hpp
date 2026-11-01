#pragma once
#include "vidra/ui/widget.hpp"

namespace vidra::ui {

class Button : public Widget {
 public:
  explicit Button(std::string label);
  void layout(RectF b) override;
  void paint(Painter& p, const Palette& pal) override;
  bool onPointer(float x, float y, bool pressed, int b) override;
  std::function<void()> onClick;
 private:
  std::string label_;
};

class IconButton : public Widget {
 public:
  explicit IconButton(std::string glyph, std::string tip = {});
  void paint(Painter& p, const Palette& pal) override;
  bool onPointer(float x, float y, bool pressed, int b) override;
  std::function<void()> onClick;
 private:
  std::string glyph_, tip_;
};

class Chip : public Widget {
 public:
  explicit Chip(std::string label);
  void layout(RectF b) override;
  void paint(Painter& p, const Palette& pal) override;
  bool onPointer(float x, float y, bool pressed, int b) override;
  std::function<void(bool)> onToggle;
 private:
  std::string label_;
};

class Toggle : public Widget {
 public:
  Toggle();
  void paint(Painter& p, const Palette& pal) override;
  bool onPointer(float x, float y, bool pressed, int b) override;
  std::function<void(bool)> onChange;
 private:
  motion::Spring knob{500, 0.72f, 1.0f};
};

class Slider : public Widget {
 public:
  Slider(float min, float max, float value);
  void paint(Painter& p, const Palette& pal) override;
  bool onPointer(float x, float y, bool pressed, int b) override;
  bool consumesDrag() const override { return true; }
  float value() const { return value_; }
  std::function<void(float)> onChange;
 private:
  float min_, max_, value_;
};

class Field : public Widget {
 public:
  explicit Field(std::string placeholder);
  void paint(Painter& p, const Palette& pal) override;
  bool onKey(const Key& k, bool pressed) override;
  bool onPointer(float x, float y, bool pressed, int b) override;
  std::string value() const { return value_; }
  void setValue(std::string v);
  std::function<void(const std::string&)> onInput;
 private:
  std::string ph_, value_;
  motion::Spring caret_{380, 0.78f, 1.0f};
  float caretT_ = 0;
};

class Card : public Widget {
 public:
  Card(std::string title, std::string author, std::string glyph);
  void layout(RectF b) override;
  void paint(Painter& p, const Palette& pal) override;
  bool onPointer(float x, float y, bool pressed, int b) override;
  const std::string& title() const { return title_; }
  std::function<void()> onOpen;
  std::function<void()> onFav;
  void setFav(bool f) { fav_ = f; }
 private:
  std::string title_, author_, glyph_;
  bool fav_ = false;
};

class Snackbar : public Widget {
 public:
  void show(std::string msg);
  void update(double dt, bool reduce) override;
  void paint(Painter& p, const Palette& pal) override;
  bool visibleNow() const { return life_ > 0; }
 private:
  std::string msg_;
  float life_ = 0;
};

class Dialog : public Widget {
 public:
  Dialog(std::string title, std::string body);
  void paint(Painter& p, const Palette& pal) override;
  bool onPointer(float x, float y, bool pressed, int b) override;
  std::function<void(bool)> onResult;
 private:
  std::string title_, body_;
};

} // namespace vidra::ui
