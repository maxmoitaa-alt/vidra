#pragma once
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace vidra::runtime {

struct Result { bool ok = false; std::string error; std::string traceback; double ms = 0.0; };

struct Limits {
  long instructions = 60'000'000;
  size_t memory = 192u << 20;
  int wallMs = 8000;
};

class Vm {
 public:
  using OutputFn = std::function<void(bool ok, const std::string& text)>;
  explicit Vm(Limits l = {});
  ~Vm();
  void setOutput(OutputFn f) { out_ = std::move(f); }
  Result execute(const std::string& src);
  void stop();
  bool busy() const { return busy_; }

 private:
  void loop();
  struct Impl;
  std::unique_ptr<Impl> impl_;
  Limits lim_;
  OutputFn out_;
  std::atomic<bool> busy_{false};
  std::atomic<bool> stop_{false};
  std::thread th_;
  std::vector<std::string> q_;
  std::mutex mu_;
  std::condition_variable cv_;
  bool down_ = false;
};

} // namespace vidra::runtime
