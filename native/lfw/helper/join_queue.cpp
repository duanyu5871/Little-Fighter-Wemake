#include "lfw/helper/join_queue.h"

#include <limits>
#include <utility>

namespace lfw {
namespace helper {

bool JoinQueue::enqueue(const Entrant& entrant) {
  if (entrant.uid.empty()) return false;
  if (_uids.count(entrant.uid) != 0) return false;
  if (static_cast<double>(_items.size()) >= _cap) return false;
  _uids.insert(entrant.uid);
  _items.push_back(entrant);
  return true;
}

std::optional<Entrant> JoinQueue::dequeue() {
  if (_items.empty()) return std::nullopt;
  Entrant ret = _items.front();
  _items.erase(_items.begin());
  _uids.erase(ret.uid);
  return ret;
}

bool JoinQueue::remove(const std::u16string& uid) {
  size_t idx = _items.size();
  for (size_t i = 0; i < _items.size(); ++i) {
    if (_items[i].uid == uid) {
      idx = i;
      break;
    }
  }
  if (idx == _items.size()) return false;
  _items.erase(_items.begin() + static_cast<std::ptrdiff_t>(idx));
  _uids.erase(uid);
  return true;
}

void JoinQueue::clear() {
  _items.clear();
  _uids.clear();
}

Value pick_join_team(const std::vector<std::pair<std::u16string, double>>& counts,
                     const std::vector<std::pair<std::u16string, double>>& caps,
                     const std::u16string* fallen, const std::vector<std::u16string>& order) {
  const auto map_get = [](const std::vector<std::pair<std::u16string, double>>& m,
                          const std::u16string& k) -> double {
    for (const auto& kv : m) {
      if (kv.first == k) return kv.second;
    }
    return 0.0;  // `counts.get(team) ?? 0`
  };
  std::vector<std::u16string> candidates;
  double least = std::numeric_limits<double>::infinity();
  for (const std::u16string& team : order) {
    const double alive = map_get(counts, team);
    if (alive <= 0) continue;
    if (map_get(caps, team) - alive <= 0) continue;
    if (alive < least) {
      least = alive;
      candidates.clear();
      candidates.push_back(team);
    } else if (alive == least) {
      candidates.push_back(team);
    }
  }
  if (candidates.empty()) return Value();
  if (fallen != nullptr && !fallen->empty()) {
    for (const std::u16string& c : candidates) {
      if (c == *fallen) return Value(*fallen);
    }
  }
  return Value(candidates[0]);
}

}  // namespace helper
}  // namespace lfw

