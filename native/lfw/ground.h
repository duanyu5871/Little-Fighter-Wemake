#pragma once

#include <optional>
#include <vector>

#include "lfw/defines/i_terrain_info.h"
#include "lfw/utils/math/line_plane_intersection.h"

namespace lfw {

struct BlockPoint {
  double x = 0.0;
  double z = 0.0;
};

struct GroundHit {
  Vec3 point{0.0, 0.0, 0.0};
  ITerrainInfo segment;
};

class Ground {
 public:
  static const ITerrainInfo& horizon();
  static const ITerrainInfo& abyss();

  void set_terrain(const std::vector<ITerrainInfo>* terrain) { _terrain = terrain; }

  double step() const { return _step; }
  const ITerrainInfo& base() const { return horizon(); }

  const ITerrainInfo& segment(double x, double z) const;
  static double y(const ITerrainInfo& seg, double x, double z);
  std::optional<double> enterable(const ITerrainInfo& seg, double x, double y, double z) const;
  const std::vector<BlockPoint>& block(const ITerrainInfo& seg, double x, double y, double z,
                                       std::optional<double> prev_x = std::nullopt,
                                       std::optional<double> prev_y = std::nullopt,
                                       std::optional<double> prev_z = std::nullopt);
  GroundHit intersect(double x1, double y1, double z1, double x2, double y2, double z2);
  std::optional<BlockPoint> intersect_wall(double x1, double y1, double z1, double x2, double y2,
                                           double z2) const;

 private:
  const std::vector<ITerrainInfo>* _terrain = nullptr;
  double _step = 10.0;
  std::vector<BlockPoint> _ret = std::vector<BlockPoint>(4);
  std::vector<BlockPoint> _empty;
  GroundHit _hit;
};

}
