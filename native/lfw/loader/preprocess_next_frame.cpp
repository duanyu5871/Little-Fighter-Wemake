#include "lfw/loader/preprocess_next_frame.h"

#include <unordered_map>
#include <variant>

#include "lfw/core/value.h"
#include "lfw/entity/entity.h"
#include "lfw/loader/get_val_from_entity.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace loader {

namespace {

bool is_nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

// `nf.__judger`：编译一次的结果（TS 是挂在 `nf` 上的一个 `Expression` 实例）。
struct Judger {
  std::u16string source;
  Expression<Entity> expr;
};

// 身份 → 判定器。`Value` 的对象是 `shared_ptr`，拷贝共享同一个 `Object`，所以按对象指针登记
// 与 TS「挂在对象上」等价；用「源串也相同」双重确认，指针被复用也不会认错。
std::unordered_map<const Object*, Judger>& judger_registry() {
  static std::unordered_map<const Object*, Judger> registry;
  return registry;
}

}

bool preprocess_next_frame(Value& nf) {
  const Array* a = as_array(nf);
  if (a != nullptr) {
    const size_t n = a->size();
    for (size_t i = 0; i < n; ++i) {
      Value item = a->at(i);
      if (!preprocess_next_frame(item)) return false;
    }
    return true;
  }
  if (is_nullish(nf)) return false;
  // TS: `if (typeof nf.expression !== "string") return nf;` ⇒ 只有字符串才挂判定器。
  const Object* const object = as_object(nf);
  if (object != nullptr) {
    const Value* const src = object->get(u"expression");
    if (src != nullptr) {
      const std::u16string* const text = std::get_if<std::u16string>(src);
      if (text != nullptr) {
        // `insert_or_assign`：对象地址可能被**释放后复用的**新对象占用（端口没有析构钩子），
        // 覆盖登记才能保证「新对象的判定器」不会被旧登记顶掉（TS 是每次 `preprocess_next_frame`
        // 重建 `nf.__judger`，语义相同）。
        judger_registry().insert_or_assign(
            object, Judger{*text, Expression<Entity>(*text, &get_val_getter_from_entity)});
      }
    }
  }
  return true;
}

Expression<Entity>* judger_of(const Value& nf) {
  const Object* const object = as_object(nf);
  if (object == nullptr) return nullptr;
  const auto it = judger_registry().find(object);
  if (it == judger_registry().end()) return nullptr;
  const Value& src = field_or(nf, u"expression");
  const std::u16string* const text = std::get_if<std::u16string>(&src);
  if (text == nullptr || *text != it->second.source) return nullptr;
  return &it->second.expr;
}

}
}
