#include "check.hpp"
#include "vidra/ui/editor.hpp"
int main() {
  vidra::ui::Buffer b;
  b.insert("local x");
  CHECK_EQ(b.all(), "local x");
  b.newline();
  b.insert("print(x)");
  CHECK_EQ(b.lines.size(), size_t(2));
  b.backspace();
  CHECK_EQ(b.all(), "local x\nprint(x");
  b.undoLast();
  CHECK_EQ(b.all(), "local x\nprint(x)");
  b.move(1, 0, false);
  CHECK_EQ(b.line, size_t(1));
  return report("editor");
}
