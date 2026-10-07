#include "lfw/dat_translator/xml/xml_to_world_dataset.h"

#include <variant>

#include "lfw/dat_translator/xml/xml_util.h"
#include "lfw/defines/fields_gen.h"

namespace lfw {
namespace dat_translator {
namespace xml {

Value xml_to_world_dataset(const IXMLElement* el) {
  auto ret = std::make_shared<Object>();
  if (el != nullptr) {
    const Object* const table = as_object(world_dataset_fields());
    for (IXMLElement* child : el->children()) {
      const std::optional<std::u16string> key = child->attr(u"name");
      if (!key || key->empty()) continue;
      const Value* const field = table != nullptr ? table->get(*key) : nullptr;
      if (field == nullptr) continue;
      const Object* const fo = as_object(*field);
      const Value* const type = fo != nullptr ? fo->get(u"type") : nullptr;
      const std::u16string* const ts =
          type != nullptr ? std::get_if<std::u16string>(type) : nullptr;
      if (ts == nullptr) continue;
      if (*ts == u"int" || *ts == u"float") {
        // TS `child.as_number()`；tool 实现里恒 undefined（见头文件说明）。
        ret->set(*key, from_opt(child->as_number()));
      } else if (*ts == u"string") {
        ret->set(*key, from_opt(child->as_string()));
      } else if (*ts == u"boolean") {
        ret->set(*key, child->as_value());
      }
    }
  }
  return Value(std::move(ret));
}

}
}
}
