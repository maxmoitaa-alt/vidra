#include "vidra/app/app.hpp"
#include "vidra/core/log.hpp"
#include <cstdlib>

int main(int, char**) {
  VIDRA_LOGI("vidra 1.0.1");
  try {
    vidra::app::App app;
    return app.run();
  } catch (const std::exception& e) {
    VIDRA_LOGE("fatal: %s", e.what());
    return 1;
  }
}
