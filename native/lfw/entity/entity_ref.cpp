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
  return Value(std::make_shared<Object>(o));
}

}
