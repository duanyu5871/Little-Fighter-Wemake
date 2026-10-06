#pragma once

#include "lfw/core/value.h"

namespace lfw {

class Background;

// TS `bg/Layer.ts`。`info` 是 `IBgLayerInfo` ⇒ 端口是 `Value`（只读它用到的字段）。
class Layer {
 public:
  Layer(Background* bg, Value info, double data_index = 0.0, double loop_index = -1.0);

  Background* bg() const { return _bg; }
  const Value& info() const { return _info; }
  // 数据文件里的下标（`data.layers` 的位置）。
  double data_index() const { return _data_index; }
  // `loop` 派生的第几个副本（`>= 0` 是 loop 副本，`-1` 是普通单层）。
  double loop_index() const { return _loop_index; }

  bool visible() const { return _visible; }
  void set_visible(bool v) { _visible = v; }

  bool is_static() const;
  void update(double count);

 private:
  Background* _bg = nullptr;
  Value _info;
  double _data_index = 0.0;
  double _loop_index = -1.0;
  bool _visible = false;
};

}
