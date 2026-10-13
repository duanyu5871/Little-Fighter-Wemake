// 台面共享假件的**实现**（切片 4CL 收尾）：只放「会拉进宿主/平台头」的那几件。
//
// 为什么单独一个 TU：`host/headless_host.h` 会拉进 `<filesystem>` 等 MSVC 内部头，而 subject TU
// 的 RTTI / 异常开关与 `lfw_core` / `lfw_headless_host` 目标不同 —— 混在同一个 TU 里会撞出
// `__msvc_heap_algorithms.hpp` 里 `_Iter_diff_t` 那种内建错误。把它们关在这里（本 TU 用与
// `lfw_core` 一致的标志编译，见 tests/differential/CMakeLists.txt），subject 的 TU 就干净了。
// 先包含台面头（它自己带 `lfw/core/value.h` 等前置），再包含宿主头 —— 反过来的话
// `host/headless_host.h` → `lfw/lfw.h` → `i_zip_object.h` 会在 `Value` 声明之前解析。
#include "tests/differential/subjects/world_fixture.h"

#include "host/headless_host.h"

namespace fx {

// `lfw::host::HeadlessHost` 的静音版：`warn/error/log/debug_msg` 不打印（case 的 stdout 是差分
// 比较对象，不能被宿主日志污染），其它一字不改。
class StubHost : public lfw::host::HeadlessHost {
 public:
  StubHost() : lfw::host::HeadlessHost({}, false) {}

  void warn(const std::vector<Value>&) override {}
  void error(const std::vector<Value>&) override {}
  void log(const std::vector<Value>&) override {}
  void debug_msg(const std::vector<Value>&) override {}
};

lfw::ILfwHost& shared_stub_host() {
  static StubHost h;
  return h;
}

lfw::LFW& shared_real_lfw() {
  static lfw::LFW l(shared_stub_host());
  return l;
}

TestWorld& shared_real_lfw_world() {
  static TestWorld w(shared_real_lfw(), g_renderer, &g_states);
  return w;
}

}  // namespace fx
