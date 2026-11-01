#include "check.hpp"
#include "vidra/gfx/sdf.hpp"
int main() {
  using namespace vidra::gfx;
  CHECK(inside({0, 0, 100, 100}, 28.0f, 4.0f, 50, 50));
  CHECK(!inside({0, 0, 100, 100}, 28.0f, 4.0f, 5, 5));
  CHECK(sdRoundBox({0, 0}, {50, 50}, 10.0f) < 0.0f);
  return report("sdf");
}
