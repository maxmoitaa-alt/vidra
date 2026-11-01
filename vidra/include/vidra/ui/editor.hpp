#pragma once
#include "vidra/ui/widget.hpp"
#include <filesystem>
#include <string>
#include <vector>

namespace vidra::ui {

struct Buffer {
  std::vector<std::string> lines{""};
  size_t line = 0, col = 0;
  size_t anchorLine = 0, anchorCol = 0;
  bool hasAnchor = false;
  std::vector<std::pair<std::vector<std::string>, std::pair<size_t, size_t>>> undo;
  std::string path;

  void insert(const std::string& utf8);
  void newline();
  void backspace();
  void del();
  void move(int dLine, int dCol, bool select);
  void undoLast();
  std::string all() const;
  bool load(const std::string& p);
  bool save(const std::string& p);
};

class Editor : public Widget {
 public:
  Editor();
  void paint(Painter& p, const Palette& pal) override;
  bool onKey(const Key& k, bool pressed) override;
  bool onPointer(float x, float y, bool pressed, int b) override;
  bool consumesDrag() const override { return true; }
  Buffer& buffer() { return buf_; }
  std::function<void(const std::string&)> onSave;
  void openFile(const std::string& path);

 private:
  Buffer buf_;
  float scroll_ = 0;
  float lineH_ = 22.0f;
  bool dragging_ = false;
  void keyDefault(const Key& k);
};

} // namespace vidra::ui
