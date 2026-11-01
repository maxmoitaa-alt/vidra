#include "check.hpp"
#include "vidra/motion/spring.hpp"
int main() {
  vidra::motion::Spring s;
  s.snap(0.0f); s.to(100.0f);
  for (int i = 0; i < 600 && !s.settled; ++i) s.step(1.0 / 120.0);
  CHECK(s.settled);
  CHECK(s.x > 99.0f);
  return report("spring");
}
