#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace vidra::platform {

enum class Role { Main, Settings, About, Console, Editor, Explorer, Appearance, Dialog };

struct Rect { int x, y, w, h; };
struct Event {
  enum class Type { Move, Button, Scroll, Key, Resize, Close, Focus } type;
  float x = 0, y = 0, dx = 0, dy = 0;
  int button = 0, key = 0;
  uint32_t mods = 0;
  bool pressed = false;
  std::string text;
};

class Window {
 public:
  virtual ~Window() = default;
  virtual Role role() const = 0;
  virtual Rect geometry() const = 0;
  virtual void* nativeDisplay() const = 0;
  virtual uint64_t nativeHandle() const = 0;
  virtual void setTitle(const std::string& t) = 0;
  virtual void resize(int w, int h) = 0;
  virtual void move(int x, int y) = 0;
  virtual void minimize() = 0;
  virtual void maximize() = 0;
  virtual void raise() = 0;
  virtual void close() = 0;
  virtual void setInputRegion(const int* rects, int count) = 0;
  virtual void pump(std::function<void(const Event&)> h) = 0;
  virtual bool shouldClose() const = 0;
};

std::unique_ptr<Window> createWindow(Role role, const std::string& title, int w, int h);

} // namespace vidra::platform
