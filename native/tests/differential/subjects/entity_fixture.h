#pragma once

// 切片 4BY 台面：`buff` 系统以 `lfw::Entity` 为准入（`buff::IBuffEntity` 已退役）⇒
// 台面假实体不再实现窄接口，而是**真 `Entity`**（或其派生）。这里放 `fx::StubEntity`：
// 只把 `dataset` 转给回调的最小 `Entity` 派生 —— `buff` 侧对宿主唯一的读面就是 `dataset`，
// 够用的台面直接用它。
// 需要逐字复刻 TS 侧日志的台面（`buff` 家族）各自派生 `lfw::Entity` 并覆写实体面。
//
// 切片 4CJ：`IEntityHost` 退役后实体的宿主就是真 `lfw::World` ⇒ 本头文件并到
// `world_fixture.h` 上（`fx::shared_world()` 与 `fx::TestWorld` 都从那儿来），原来那个
// 「最小假宿主 `fx::Host`」随之删除。

#include <functional>
#include <string>
#include <utility>

#include "lfw/buff/buff.h"
#include "lfw/entity/entity.h"

#include "world_fixture.h"

namespace fx {

// 一个真 `Entity`：构造后把 `id` 钉成调用方给的值（`Entity::reset` 会用世界的
// `new_id` 覆盖它），`dataset` 转给回调（缺省空值）。
class StubEntity : public lfw::Entity {
 public:
  explicit StubEntity(std::u16string id, lfw::Value data = lfw::Value())
      : lfw::Entity(shared_world(), std::move(data)) {
    this->id = std::move(id);
  }

  std::function<lfw::Value(const std::u16string&)> on_dataset;

  lfw::Value dataset(const std::u16string& key) const override;
};

inline lfw::Value StubEntity::dataset(const std::u16string& key) const {
  return on_dataset ? on_dataset(key) : lfw::Value();
}

}  // namespace fx
