#pragma once

#include "lfw/helper/entities_helper.h"

namespace lfw {
namespace helper {

// TS `helper/BallsHelper.ts`。
class BallsHelper : public ObjectsHelper {
 public:
  using ObjectsHelper::ObjectsHelper;
  std::vector<Entity*> all() const override;
};

}  // namespace helper
}  // namespace lfw
