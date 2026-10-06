#include "lfw/entity/entity_ref.h"

#include <memory>
#include <string>

#include "lfw/core/value.h"
#include "lfw/entity/entity.h"

namespace lfw {

Value ref_of(const Entity& e) {
  Object pos;
  pos.set(u"x", Value(e.position.x));
  pos.set(u"y", Value(e.position.y));
  pos.set(u"z", Value(e.position.z));
  Object o;
  o.set(u"id", Value(e.id));
  o.set(u"position", Value(std::make_shared<Object>(pos)));
  o.set(u"frame", e.frame);
  o.set(u"team", Value(e.team()));
  o.set(u"hp", Value(e.hp()));
  o.set(u"type", Value(e.type()));
  o.set(u"ghosted", Value(e.ghosted()));
  // `entity::is_fighter(ref)` / `is_weapon` / `is_ball` 读的是 `ref.data.type`（TS 的
  // `is_fighter(e)` 就是 `e.data.type === Fighter`）⇒ 引用上必须带上 `data` 本身，否则
  // 「这份引用代表的是不是战士」永远为假。
  o.set(u"data", e.data());
  return Value(std::make_shared<Object>(o));
}

}
