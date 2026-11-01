#include "check.hpp"
#include "vidra/runtime/vm.hpp"
#include <chrono>
#include <thread>
int main() {
  vidra::runtime::Vm vm;
  bool done = false;
  vm.setOutput([&](bool, const std::string&) { done = true; });
  vm.execute("local x = 0 for i = 1, 1000 do x = x + i end assert(x == 500500)");
  for (int i = 0; i < 300 && !done; ++i) std::this_thread::sleep_for(std::chrono::milliseconds(10));
  CHECK(done);
  return report("runtime");
}
