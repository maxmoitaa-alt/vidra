#pragma once
#include "vidra/color/palette.hpp"
#include "vidra/core/settings.hpp"
#include "vidra/gfx/renderer.hpp"
#include "vidra/gfx/text.hpp"
#include "vidra/platform/window.hpp"
#include "vidra/runtime/vm.hpp"
#include "vidra/ui/root.hpp"
#include <map>
#include <memory>

namespace vidra::app {

struct Slot {
  std::unique_ptr<platform::Window> win;
  std::unique_ptr<gfx::Renderer> ren;
  std::unique_ptr<gfx::TextEngine> text;
  std::unique_ptr<ui::Painter> painter;
  std::unique_ptr<ui::Root> root;
  bool alive = true;
  int w = 0, h = 0;
};

class App {
 public:
  App();
  ~App();
  int run();

 private:
  void open(platform::Role r);
  void close(platform::Role r);
  void buildMain(Slot& s);
  void buildSettings(Slot& s);
  void buildAbout(Slot& s);
  void buildConsole(Slot& s);
  void buildEditor(Slot& s, const std::string& title, const std::string& src);
  void buildExplorer(Slot& s);
  void buildAppearance(Slot& s);
  void tickSlot(platform::Role r, Slot& s, double dt);
  void saveStates();
  void loadStates();

  core::Settings& st_;
  runtime::Vm vm_;
  std::map<platform::Role, Slot> slots_;
  color::Palette pal_;
  float crossfade_ = 1.0f;
  bool running_ = true;
  double t_ = 0;
};

} // namespace vidra::app
