#pragma once

#include <array>
#include <optional>

namespace lfw {

std::optional<std::array<double, 2>> project_to_line(double x, double y, double m, double n);

}
