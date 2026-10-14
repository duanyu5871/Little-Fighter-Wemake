#include "lfw/base/debugging.h"

namespace lfw {

void debugging_write(IDebugging& obj, DebugLevel level, const std::u16string& func,
                     const std::vector<Value>& args) {
  // TS：还没被 `make_debugging(this)` 替换过的实例上，这三条是基类的空转。
  if (!obj.debugging_attached) return;

  // TS 的 `debug` 闭包第一句：`if (!obj.__debugging) return;`（`warn` / `log` 没有这句）。
  if (level == DebugLevel::Debug && !obj.debugging_enabled()) return;

  std::u16string head = u"[";
  head.push_back(level == DebugLevel::Debug ? u'D' : (level == DebugLevel::Warn ? u'W' : u'I'));
  head += u"]";
  const std::u16string& id = obj.debugging_id();
  if (!id.empty()) {
    head += u"<";
    head += id;
    head += u">";
  }
  head += u"[";
  head += obj.debugging_tag();
  head += u"::";
  head += func;
  head += u"]";

  std::vector<Value> out;
  out.reserve(args.size() + 1);
  out.push_back(Value(head));
  for (const Value& a : args) out.push_back(a);
  obj.debugging_out(level, out);
}

}
