#include "lfw/dat_translator/xml/xml_from_world_dataset.h"

#include <utility>
#include <variant>
#include <vector>

#include "lfw/defines/fields_gen.h"
#include "lfw/utils/container_help/field_or.h"

namespace lfw {
namespace dat_translator {
namespace xml {

std::shared_ptr<IXMLElement> xml_from_world_dataset(IXML& xml, const Value& data,
                                                    const std::u16string& tag) {
  std::vector<std::pair<std::u16string, Value>> item;
  const Object* const table = as_object(world_dataset_fields());
  if (table != nullptr) {
    for (const std::u16string& k : table->keys()) {
      const Value* const v = [&]() -> const Value* {
        const Object* const o = as_object(data);
        return o != nullptr ? o->get(k) : nullptr;
      }();
      if (v == nullptr) continue;
      if (std::holds_alternative<std::monostate>(*v)) continue;  // `v !== void 0`
      item.emplace_back(k, *v);
    }
  }
  if (item.empty()) return nullptr;
  auto obj = std::make_shared<Object>();
  for (const std::pair<std::u16string, Value>& kv : item) obj->set(kv.first, kv.second);
  return xml.from_object(Value(std::move(obj)), tag);
}

}
}
}
