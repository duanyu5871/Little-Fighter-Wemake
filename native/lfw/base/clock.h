#pragma once

namespace lfw {

class IClock {
 public:
  virtual ~IClock() = default;
  virtual double now_ms() const = 0;
};

inline IClock*& clock_slot() {
  static IClock* slot = nullptr;
  return slot;
}

inline IClock* clock() { return clock_slot(); }

inline void set_clock(IClock* value) { clock_slot() = value; }

}
