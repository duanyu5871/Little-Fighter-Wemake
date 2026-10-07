#include "lfw/entity/entity_ref.h"

#include <memory>
#include <string>

#include "lfw/core/value.h"
#include "lfw/entity/entity.h"

namespace lfw {
namespace {

Value ref_impl(const Entity& e, bool with_relations);

// 关系槽（`holding` / `catching` / `bearer` / `catcher`）里的引用只带一层：不再递归带
// 它们自己的关系。TS 的读法（`e.bearer?.frame.wpoint`）用这一层就够。
Value shallow_ref(const Entity& e) { return ref_impl(e, false); }

Value ref_impl(const Entity& e, bool with_relations) {
  Object pos;
  pos.set(u"x", Value(e.position.x));
  pos.set(u"y", Value(e.position.y));
  pos.set(u"z", Value(e.position.z));
  Object vel;
  vel.set(u"x", Value(e.velocity.x));
  vel.set(u"y", Value(e.velocity.y));
  vel.set(u"z", Value(e.velocity.z));
  Object o;
  o.set(u"id", Value(e.id));
  o.set(u"position", Value(std::make_shared<Object>(pos)));
  o.set(u"velocity", Value(std::make_shared<Object>(vel)));
  o.set(u"frame", e.frame);
  o.set(u"team", Value(e.team()));
  o.set(u"hp", Value(e.hp()));
  o.set(u"type", Value(e.type()));
  o.set(u"ghosted", Value(e.ghosted()));
  // `entity::is_fighter(ref)` / `is_weapon` / `is_ball` 读的是 `ref.data.type`（TS 的
  // `is_fighter(e)` 就是 `e.data.type === Fighter`）⇒ 引用上必须带上 `data` 本身，否则
  // 「这份引用代表的是不是战士」永远为假。`bot_ignore` / `base_type` 这类 getter 也可以
  // 从 `frame` + `data` 现算。
  o.set(u"data", e.data());
  // ---- `bot/*` 读**对家**的那批字段（TS 里全是活实体的 getter）----
  o.set(u"facing", Value(e.facing));
  o.set(u"mounted", Value(e.mounted()));
  o.set(u"invisible", Value(e.invisible()));
  o.set(u"invulnerable", Value(e.invulnerable()));
  o.set(u"toughness", Value(e.toughness()));
  o.set(u"arest", Value(e.arest()));
  o.set(u"wakeup_invuln", Value(e.wakeup_invuln));
  o.set(u"resting", Value(e.resting()));
  o.set(u"ground_y", Value(e.ground_y()));
  o.set(u"is_on_ground", Value(e.is_on_ground));
  if (with_relations) {
    o.set(u"holding", e.holding != nullptr ? shallow_ref(*e.holding) : Value(NullTag{}));
    o.set(u"catching", e.catching != nullptr ? shallow_ref(*e.catching) : Value(NullTag{}));
    o.set(u"bearer", e.bearer != nullptr ? shallow_ref(*e.bearer) : Value(NullTag{}));
    o.set(u"catcher", e.catcher != nullptr ? shallow_ref(*e.catcher) : Value(NullTag{}));
  }
  return Value(std::make_shared<Object>(o));
}

}  // namespace

Value ref_of(const Entity& e) { return ref_impl(e, true); }

}
