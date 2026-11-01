#pragma once
#include "vidra/core/settings.hpp"
#include "vidra/runtime/vm.hpp"
#include "vidra/ui/editor.hpp"
#include "vidra/ui/widgets.hpp"
#include <deque>

namespace vidra::ui {

struct ScriptEntry { std::string name, author, glyph, source, category; };

class Home : public Widget {
 public:
  Home(core::Settings& s, runtime::Vm& vm);
  void layout(RectF b) override;
  void paint(Painter& p, const Palette& pal) override;
  void update(double dt, bool reduce) override;
  std::function<void(const std::string&, const std::string&)> onOpen;
 private:
  core::Settings& st_;
  runtime::Vm& vm_;
  std::unique_ptr<Field> search_;
  std::vector<std::unique_ptr<Chip>> chips_;
  std::vector<std::unique_ptr<Card>> cards_;
  std::vector<ScriptEntry> all_;
  std::string filter_;
  std::string category_;
  void rebuild();
};

class ConsoleView : public Widget {
 public:
  explicit ConsoleView(runtime::Vm& vm);
  void update(double dt, bool reduce) override;
  void paint(Painter& p, const Palette& pal) override;
  void append(bool err, const std::string& text);
 private:
  runtime::Vm& vm_;
  std::deque<std::pair<bool, std::string>> lines_;
  float scroll_ = 0;
};

class Explorer : public Widget {
 public:
  Explorer();
  void paint(Painter& p, const Palette& pal) override;
  bool onPointer(float x, float y, bool pressed, int b) override;
  void layout(RectF b) override;
 private:
  struct Node { std::string name; int depth; bool open; std::vector<int> kids; };
  std::vector<Node> nodes_;
  std::vector<int> visible_;
  void flatten(int idx);
  void build();
};

class SettingsView : public Widget {
 public:
  explicit SettingsView(core::Settings& s);
  void layout(RectF b) override;
  void paint(Painter& p, const Palette& pal) override;
  std::function<void()> onThemeChange;
 private:
  core::Settings& st_;
  std::vector<std::unique_ptr<Widget>> rows_;
};

class AboutView : public Widget {
 public:
  AboutView();
  void paint(Painter& p, const Palette& pal) override;
};

} // namespace vidra::ui
