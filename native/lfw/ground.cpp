#include "ground.h"

#include <cmath>
#include <limits>

#include "lfw/utils/math/base.h"
#include "lfw/utils/math/clamp.h"

namespace lfw {

const ITerrainInfo& Ground::horizon() {
  static const ITerrainInfo t = [] {
    ITerrainInfo r;
    r.id = u"horizon_0";
    r.name = u"horizon_0";
    r.type = static_cast<int>(TerrainEnum::Flat);
    r.x1 = -9007199254740991.0;
    r.x2 = 9007199254740991.0;
    r.z1 = -9007199254740991.0;
    r.z2 = 9007199254740991.0;
    r.h1 = 0.0;
    r.h2 = 0.0;
    return r;
  }();
  return t;
}

const ITerrainInfo& Ground::abyss() {
  static const ITerrainInfo t = [] {
    ITerrainInfo r;
    r.id = u"ABYSS_0";
    r.name = u"ABYSS_0";
    r.type = static_cast<int>(TerrainEnum::Flat);
    r.x1 = -9007199254740991.0;
    r.x2 = 9007199254740991.0;
    r.z1 = -9007199254740991.0;
    r.z2 = 9007199254740991.0;
    r.h1 = -9007199254740991.0;
    r.h2 = -9007199254740991.0;
    return r;
  }();
  return t;
}

const ITerrainInfo& Ground::segment(double x, double z) const {
  if (_terrain != nullptr && !_terrain->empty()) {
    const ITerrainInfo* best = nullptr;
    double best_y = 0.0;
    bool has_best = false;
    for (const ITerrainInfo& seg : *_terrain) {
      if (x < seg.x1 || x > seg.x2) continue;
      if (z < seg.z1 || z > seg.z2) continue;
      const double seg_y = y(seg, x, z);
      if (!has_best || seg_y > best_y) {
        best_y = seg_y;
        best = &seg;
        has_best = true;
      }
    }
    if (best != nullptr) return *best;
  }
  return base();
}

double Ground::y(const ITerrainInfo& seg, double x, double z) {
  switch (seg.type) {
    case static_cast<int>(TerrainEnum::Flat):
      return seg.h1;
    case static_cast<int>(TerrainEnum::SlopeH): {
      x = clamp(x, seg.x1, seg.x2);
      z = clamp(z, seg.z1, seg.z2);
      const double t = (x - seg.x1) / (seg.x2 - seg.x1);
      return seg.h1 + t * (seg.h2 - seg.h1);
    }
    case static_cast<int>(TerrainEnum::SlopeV): {
      x = clamp(x, seg.x1, seg.x2);
      z = clamp(z, seg.z1, seg.z2);
      const double t = (z - seg.z1) / (seg.z2 - seg.z1);
      return seg.h1 + t * (seg.h2 - seg.h1);
    }
  }
  return 0.0;
}

std::optional<double> Ground::enterable(const ITerrainInfo& seg, double x, double y_value,
                                        double z) const {
  const double dist_y = y(seg, x, z);
  const double diff_h = dist_y - y_value;
  if (diff_h > _step) return std::nullopt;
  return dist_y;
}

const std::vector<BlockPoint>& Ground::block(const ITerrainInfo& seg, double x, double y_value,
                                             double z, std::optional<double> prev_x,
                                             std::optional<double> prev_y,
                                             std::optional<double> prev_z) {
  if (seg.id == base().id) return _empty;

  const double px = prev_x.value_or(x);
  const double pz = prev_z.value_or(z);
  (void)prev_y;

  double l = seg.x1 - 1.0;
  double r = seg.x2 + 1.0;
  double f = seg.z1 - 1.0;
  double n = seg.z2 + 1.0;

  const double mid_x = (seg.x1 + seg.x2) / 2.0;
  const double mid_z = (seg.z1 + seg.z2) / 2.0;
  std::optional<double> slope_x;
  std::optional<double> slope_z;

  switch (seg.type) {
    case static_cast<int>(TerrainEnum::SlopeH): {
      const double dh = seg.h2 - seg.h1;
      if (dh == 0.0) break;
      const double t = (y_value - seg.h1) / dh;
      slope_x = seg.x1 + clamp(t, 0.0, 1.0) * (seg.x2 - seg.x1);
      const double sx = clamp(seg.x1 + t * (seg.x2 - seg.x1), l, r);
      if (sx > x) r = sx + 1.0;
      else l = sx - 1.0;
      break;
    }
    case static_cast<int>(TerrainEnum::SlopeV): {
      const double dh = seg.h2 - seg.h1;
      if (dh == 0.0) break;
      const double t = (y_value - seg.h1) / dh;
      slope_z = seg.z1 + clamp(t, 0.0, 1.0) * (seg.z2 - seg.z1);
      const double sz = clamp(seg.z1 + t * (seg.z2 - seg.z1), f, n);
      if (sz > z) n = sz + 1.0;
      else f = sz - 1.0;
      break;
    }
    default:
      break;
  }

  const bool from_l = px <= (slope_x.has_value() ? *slope_x : mid_x);
  const bool from_f = pz <= (slope_z.has_value() ? *slope_z : mid_z);
  const double fx = from_l ? l : r;
  const double ox = from_l ? r : l;
  const double fz = from_f ? f : n;
  const double oz = from_f ? n : f;

  if (abs(fx - x) < abs(fz - z)) {
    _ret[0].x = fx;
    _ret[0].z = z;
    _ret[1].x = x;
    _ret[1].z = fz;
  } else {
    _ret[0].x = x;
    _ret[0].z = fz;
    _ret[1].x = fx;
    _ret[1].z = z;
  }

  if (abs(ox - x) < abs(oz - z)) {
    _ret[2].x = ox;
    _ret[2].z = z;
    _ret[3].x = x;
    _ret[3].z = oz;
  } else {
    _ret[2].x = x;
    _ret[2].z = oz;
    _ret[3].x = ox;
    _ret[3].z = z;
  }
  return _ret;
}

GroundHit Ground::intersect(double x1, double y1, double z1, double x2, double y2, double z2) {
  double best_t = std::numeric_limits<double>::infinity();
  const double dx = x2 - x1;
  const double dy = y2 - y1;
  const double dz = z2 - z1;

  if (_terrain != nullptr && !_terrain->empty()) {
    for (const ITerrainInfo& seg : *_terrain) {
      double a = 0.0;
      double b = 0.0;
      double c = 0.0;
      double d = 0.0;
      switch (seg.type) {
        case static_cast<int>(TerrainEnum::Flat):
          a = 0.0;
          b = 1.0;
          c = 0.0;
          d = -seg.h1;
          break;
        case static_cast<int>(TerrainEnum::SlopeH): {
          const double dh = seg.h2 - seg.h1;
          const double dx2 = seg.x2 - seg.x1;
          a = -dh;
          b = dx2;
          c = 0.0;
          d = seg.x1 * dh - seg.h1 * dx2;
          break;
        }
        case static_cast<int>(TerrainEnum::SlopeV): {
          const double dh = seg.h2 - seg.h1;
          const double dz2 = seg.z2 - seg.z1;
          a = 0.0;
          b = dz2;
          c = -dh;
          d = seg.z1 * dh - seg.h1 * dz2;
          break;
        }
        default:
          continue;
      }

      const Vec3* hit =
          line_plane_intersection(a, b, c, d, x1, y1, z1, x2, y2, z2, false, true);
      if (hit != nullptr) {
        if (hit->x >= seg.x1 && hit->x <= seg.x2 && hit->z >= seg.z1 && hit->z <= seg.z2) {
          const double seg_y = y(seg, hit->x, hit->z);
          if (seg_y - y1 <= _step) {
            const double dhx = hit->x - x1;
            const double dhy = hit->y - y1;
            const double dhz = hit->z - z1;
            const double denom = dx * dx + dy * dy + dz * dz;
            const double t =
                denom == 0.0 ? 0.0 : (dhx * dx + dhy * dy + dhz * dz) / denom;
            if (t > 1e-6 && t < best_t) {
              best_t = t;
              _hit.point.x = hit->x;
              _hit.point.y = hit->y;
              _hit.point.z = hit->z;
            }
          }
        }
      }

      if (dx != 0.0) {
        const double t = (seg.x1 - x1) / dx;
        if (t > 0.0 && t < 1.0 && t < best_t) {
          const double iz = z1 + t * dz;
          if (iz >= seg.z1 && iz <= seg.z2) {
            const double iy = y1 + t * dy;
            const double ty = y(seg, seg.x1, iz);
            if (ty - iy > _step) {
              best_t = t;
              _hit.point.x = seg.x1 - 1.0;
              _hit.point.y = y2;
              _hit.point.z = iz;
            }
          }
        }
      }
      if (dx != 0.0) {
        const double t = (seg.x2 - x1) / dx;
        if (t > 0.0 && t < 1.0 && t < best_t) {
          const double iz = z1 + t * dz;
          if (iz >= seg.z1 && iz <= seg.z2) {
            const double iy = y1 + t * dy;
            const double ty = y(seg, seg.x2, iz);
            if (ty - iy > _step) {
              best_t = t;
              _hit.point.x = seg.x2 + 1.0;
              _hit.point.y = y2;
              _hit.point.z = iz;
            }
          }
        }
      }
      if (dz != 0.0) {
        const double t = (seg.z1 - z1) / dz;
        if (t > 0.0 && t < 1.0 && t < best_t) {
          const double ix = x1 + t * dx;
          if (ix >= seg.x1 && ix <= seg.x2) {
            const double iy = y1 + t * dy;
            const double ty = y(seg, ix, seg.z1);
            if (ty - iy > _step) {
              best_t = t;
              _hit.point.x = ix;
              _hit.point.y = y2;
              _hit.point.z = seg.z1 - 1.0;
            }
          }
        }
      }
      if (dz != 0.0) {
        const double t = (seg.z2 - z1) / dz;
        if (t > 0.0 && t < 1.0 && t < best_t) {
          const double ix = x1 + t * dx;
          if (ix >= seg.x1 && ix <= seg.x2) {
            const double iy = y1 + t * dy;
            const double ty = y(seg, ix, seg.z2);
            if (ty - iy > _step) {
              best_t = t;
              _hit.point.x = ix;
              _hit.point.y = y2;
              _hit.point.z = seg.z2 + 1.0;
            }
          }
        }
      }
    }
  }

  if (!std::isfinite(best_t)) {
    _hit.point.x = x2;
    _hit.point.y = y2;
    _hit.point.z = z2;
  }
  _hit.segment = segment(_hit.point.x, _hit.point.z);
  return _hit;
}

std::optional<BlockPoint> Ground::intersect_wall(double x1, double y1, double z1, double x2,
                                                 double y2, double z2) const {
  if (_terrain == nullptr || _terrain->empty()) return std::nullopt;

  double best_t = std::numeric_limits<double>::infinity();
  double best_x = 0.0;
  double best_z = 0.0;
  const double dx = x2 - x1;
  const double dy = y2 - y1;
  const double dz = z2 - z1;

  if (dx == 0.0 && dz == 0.0) return std::nullopt;

  for (const ITerrainInfo& seg : *_terrain) {
    const double max_h = seg.h1 > seg.h2 ? seg.h1 : seg.h2;
    const double min_y = y1 < y2 ? y1 : y2;
    if (max_h - min_y <= _step) continue;

    if (dx != 0.0) {
      const double t = (seg.x1 - x1) / dx;
      if (t > 0.0 && t < 1.0 && t < best_t) {
        const double iz = z1 + t * dz;
        if (iz >= seg.z1 && iz <= seg.z2) {
          const double iy = y1 + t * dy;
          const double ty = y(seg, seg.x1, iz);
          if (ty - iy > _step) {
            best_t = t;
            best_x = seg.x1 - 1.0;
            best_z = z2;
          }
        }
      }
    }
    if (dx != 0.0) {
      const double t = (seg.x2 - x1) / dx;
      if (t > 0.0 && t < 1.0 && t < best_t) {
        const double iz = z1 + t * dz;
        if (iz >= seg.z1 && iz <= seg.z2) {
          const double iy = y1 + t * dy;
          const double ty = y(seg, seg.x2, iz);
          if (ty - iy > _step) {
            best_t = t;
            best_x = seg.x2 + 1.0;
            best_z = z2;
          }
        }
      }
    }
    if (dz != 0.0) {
      const double t = (seg.z1 - z1) / dz;
      if (t > 0.0 && t < 1.0 && t < best_t) {
        const double ix = x1 + t * dx;
        if (ix >= seg.x1 && ix <= seg.x2) {
          const double iy = y1 + t * dy;
          const double ty = y(seg, ix, seg.z1);
          if (ty - iy > _step) {
            best_t = t;
            best_x = x2;
            best_z = seg.z1 - 1.0;
          }
        }
      }
    }
    if (dz != 0.0) {
      const double t = (seg.z2 - z1) / dz;
      if (t > 0.0 && t < 1.0 && t < best_t) {
        const double ix = x1 + t * dx;
        if (ix >= seg.x1 && ix <= seg.x2) {
          const double iy = y1 + t * dy;
          const double ty = y(seg, ix, seg.z2);
          if (ty - iy > _step) {
            best_t = t;
            best_x = x2;
            best_z = seg.z2 + 1.0;
          }
        }
      }
    }
  }

  if (!std::isfinite(best_t)) return std::nullopt;
  BlockPoint out;
  out.x = best_x;
  out.z = best_z;
  return out;
}

}
