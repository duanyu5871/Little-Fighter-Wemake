#pragma once

#include <memory>
#include <vector>

#include "lfw/bg/layer.h"
#include "lfw/core/value.h"

namespace lfw {

class World;

// TS `bg/Background.ts`。`World` 只前置声明（本刀不构造世界）。
class Background {
 public:
  // `middle: { x, z }`：TS 是对象字面量，端口给个小结构（同 `ditto::vec2` 的先例）。
  struct Middle {
    double x = 0.0;
    double z = 0.0;
  };

  Background(World* world, Value data);

  const Value& data() const { return _data; }
  World* world() const { return _world; }
  const std::vector<std::shared_ptr<Layer>>& layers() const { return _layers; }

  const Value& id() const { return _id; }
  const Value& name() const { return _name; }
  double left() const { return _left; }
  double right() const { return _right; }
  // TS 的 `near` / `far` 是 `<windows.h>` 的宏名 ⇒ 端口加后缀。
  double near_plane() const { return _near; }
  double far_plane() const { return _far; }
  double width() const { return _width; }
  double height() const { return _height; }
  double depth() const { return _depth; }
  const Middle& middle() const { return _middle; }
  double zoom_x() const { return _zoom_x; }
  double zoom_y() const { return _zoom_y; }
  double zoom_z() const { return _zoom_z; }

  void update();
  void dispose();

  double update_times() const { return _update_times; }
  double layer_data_index() const { return _layer_data_index; }

 private:
  void add_layer(const Value& info);

  World* _world = nullptr;
  Value _data;
  std::vector<std::shared_ptr<Layer>> _layers;
  Value _id;
  Value _name;
  double _left = 0.0;
  double _right = 0.0;
  double _near = 0.0;
  double _far = 0.0;
  double _width = 0.0;
  double _height = 0.0;
  double _depth = 0.0;
  Middle _middle;
  double _zoom_x = 1.0;
  double _zoom_y = 1.0;
  double _zoom_z = 1.0;
  double _update_times = 0.0;
  double _layer_data_index = 0.0;
};

}
