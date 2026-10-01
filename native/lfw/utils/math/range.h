#pragma once

#include <optional>
#include <vector>

namespace lfw {

std::optional<std::vector<double>> range(double from, double to, double gap = 1.0);

}
