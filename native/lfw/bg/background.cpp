#include "lfw/bg/background.h"

#include <memory>
#include <utility>

#include "lfw/utils/container_help/field_or.h"
#include "lfw/utils/math/base.h"

namespace lfw {
namespace {

// `v == null`（各文件的既有写法，见 `camera.cpp` / `zip_mgr.cpp`）。
bool is_nullish(const Value& v) {
  return std::holds_alternative<std::monostate>(v) || std::holds_alternative<NullTag>(v);
}

// `v === void 0`：只认 `undefined`，`null` 不算。
bool is_undefined(const Value& v) { return std::holds_alternative<std::monostate>(v); }

// `x ?? 0`：只吞 `null` / `undefined`（`0` / `""` / `NaN` 原样走数值强转）。
double num_or(const Value& v, double or_value) {
  return is_nullish(v) ? or_value : to_number(v);
}

// `{ ...info, x }`：浅拷贝一份再改 `x`（TS 是新建对象；端口的 `Object` 是共享的，必须复制）。
Value with_x(const Value& info, double x) {
  auto out = std::make_shared<Object>();
  if (const Object* o = as_object(info)) {
    for (const std::u16string& key : o->keys()) {
      const Value* v = o->get(key);
      out->set(key, v != nullptr ? *v : Value());
    }
  }
  out->set(u"x", Value(x));
  return Value(out);
}

}

Background::Background(World* world, Value data) : _world(world), _data(std::move(data)) {
  _id = field_or(_data, u"id");

  const Value info = field_or(_data, u"base");

  const Value name_v = field_or(info, u"name");
  _name = is_nullish(name_v) ? _id : name_v;

  _left = num_or(field_or(info, u"left"), 0.0);
  _right = num_or(field_or(info, u"right"), 0.0);
  _near = num_or(field_or(info, u"near"), 0.0);
  _far = num_or(field_or(info, u"far"), 0.0);
  _width = _right - _left;
  _depth = _near - _far;
  _height = num_or(field_or(info, u"height"), 0.0);
  _middle.x = (_right + _left) / 2.0;
  _middle.z = (_far + _near) / 2.0;

  const Value layers = field_or(_data, u"layers");
  if (truthy(layers)) {
    if (const Array* arr = as_array(layers)) {
      for (size_t i = 0; i < arr->size(); ++i) add_layer(arr->at(i));
    }
  }

  _zoom_x = num_or(field_or(info, u"zoom_x"), 1.0);
  _zoom_y = num_or(field_or(info, u"zoom_y"), 1.0);
  _zoom_z = num_or(field_or(info, u"zoom_z"), 1.0);
}

void Background::add_layer(const Value& info) {
  const double data_index = _layer_data_index++;
  double x = to_number(field_or(info, u"x"));
  const Value loop_v = field_or(info, u"loop");
  const double loop = is_undefined(loop_v) ? 0.0 : to_number(loop_v);
  if (loop <= 0.0) {
    _layers.push_back(std::make_shared<Layer>(this, info, data_index, -1.0));
    return;
  }
  const double right = _width + loop;
  double loop_index = 0.0;
  for (x -= loop; x < right; x += loop) {
    _layers.push_back(std::make_shared<Layer>(this, with_x(info, x), data_index, loop_index));
    loop_index++;
  }
}

void Background::update() {
  _update_times++;
  for (const std::shared_ptr<Layer>& layer : _layers) layer->update(_update_times);
}

void Background::dispose() {
  _layers.clear();
  _layer_data_index = 0.0;
}

}
