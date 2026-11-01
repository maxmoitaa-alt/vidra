#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace vidra::core {

struct WindowState { int x = -1, y = -1, w = 0, h = 0; bool open = false; };

struct Settings {
  bool dark = true;
  bool sound = true;
  bool reduceMotion = false;
  float density = 1.0f;
  float uiScale = 1.0f;
  uint32_t seed = 0x6750A4;
  std::string renderer = "auto";
  bool autoScroll = true;
  std::vector<std::string> favorites;
  std::vector<std::string> history;
  WindowState main, settings, console, editor, explorer, appearance;

  void load();
  void save() const;
  static Settings& get();
};

} // namespace vidra::core
