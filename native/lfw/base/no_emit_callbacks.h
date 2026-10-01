#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "lfw/core/js_string.h"
#include "lfw/core/value.h"

namespace lfw {

using CallbacksWarnFn = std::function<void(const std::u16string&)>;

inline CallbacksWarnFn& callbacks_warn() {
  static CallbacksWarnFn f;
  return f;
}

constexpr size_t kMaxPendingsPerFlush = 1000;

template <typename Payload>
class NoEmitCallbacksT {
 public:
  using Payloads = std::vector<Payload>;
  using Fn = std::function<void(const Payloads&)>;
  using Remover = std::function<void()>;

  struct Handler {
    std::u16string key;
    Fn fn;
  };

  struct Listener {
    std::vector<Handler> handlers;
    bool once = false;
  };

  NoEmitCallbacksT() = default;
  NoEmitCallbacksT(const NoEmitCallbacksT&) = delete;
  NoEmitCallbacksT& operator=(const NoEmitCallbacksT&) = delete;

  void clear() { _packs.clear(); }

  Remover add(Listener* v) {
    if (v == nullptr || v->handlers.empty()) return Remover();
    for (const Handler& h : v->handlers) {
      if (h.key == u"once") continue;
      pack_for(h.key).add(v);
    }
    return [this, v]() { del(v); };
  }

  Remover on(const std::u16string& key, Fn f) {
    auto owned = std::make_unique<Listener>();
    owned->handlers.push_back(Handler{key, std::move(f)});
    owned->once = false;
    Listener* raw = owned.get();
    _owned.push_back(std::move(owned));
    return add(raw);
  }

  Remover once(const std::u16string& key, Fn f) {
    auto owned = std::make_unique<Listener>();
    owned->handlers.push_back(Handler{key, std::move(f)});
    owned->once = true;
    Listener* raw = owned.get();
    _owned.push_back(std::move(owned));
    return add(raw);
  }

  void del(Listener* v) {
    for (Pack& pack : _packs) pack.remove(v);
  }

  size_t listener_count(const std::u16string& key) const {
    for (const Pack& pack : _packs) {
      if (pack.key() == key) return pack.size();
    }
    return 0;
  }

  std::vector<std::u16string> keys() const {
    std::vector<std::u16string> out;
    for (const Pack& pack : _packs) out.push_back(pack.key());
    return out;
  }

  size_t pending_count(const std::u16string& key) const {
    for (const Pack& pack : _packs) {
      if (pack.key() == key) return pack.pending_count();
    }
    return 0;
  }

 protected:
  void emit(const std::u16string& key, const Payloads& args) {
    for (Pack& pack : _packs) {
      if (pack.key() == key) {
        pack.emit(args);
        return;
      }
    }
  }

 private:
  class Pack {
   public:
    explicit Pack(std::u16string key) : _key(std::move(key)) {}

    const std::u16string& key() const { return _key; }
    size_t size() const { return _set.size(); }
    size_t pending_count() const { return _pendings.size() - _head; }

    void add(Listener* v) {
      if (_emiting) {
        _waits.push_back({WaitKind::kAdd, v});
        return;
      }
      set_add(v);
    }

    void remove(Listener* v) {
      if (_emiting) {
        _waits.push_back({WaitKind::kDel, v});
        return;
      }
      set_remove(v);
    }

    void emit(const Payloads& args) {
      _pendings.push_back(args);
      if (_emiting) return;
      handle_pendings();
    }

   private:
    enum class WaitKind { kAdd, kDel };

    struct Wait {
      WaitKind kind;
      Listener* who;
    };

    static const Fn* find_handler(Listener* v, const std::u16string& key) {
      for (const Handler& h : v->handlers) {
        if (h.key == key) return &h.fn;
      }
      return nullptr;
    }

    void set_add(Listener* v) {
      for (Listener* x : _set) {
        if (x == v) return;
      }
      _set.push_back(v);
    }

    void set_remove(Listener* v) {
      for (size_t i = 0; i < _set.size(); ++i) {
        if (_set[i] == v) {
          _set.erase(_set.begin() + static_cast<std::ptrdiff_t>(i));
          return;
        }
      }
    }

    void handle_waits() {
      if (_waits.empty()) return;
      for (const Wait& w : _waits) {
        if (w.kind == WaitKind::kAdd) set_add(w.who);
        else set_remove(w.who);
      }
      _waits.clear();
    }

    void compact() {
      if (_head == 0) return;
      _pendings.erase(_pendings.begin(), _pendings.begin() + static_cast<std::ptrdiff_t>(_head));
      _head = 0;
    }

    void handle_pendings() {
      size_t done = 0;

      while (_head < _pendings.size()) {
        if (done >= kMaxPendingsPerFlush) {
          if (!_overflow_warned) {
            _overflow_warned = true;
            CallbacksWarnFn& warn = callbacks_warn();
            if (warn) warn(overflow_message(_key, _pendings.size() - _head));
          }
          compact();
          return;
        }
        const Payloads args = _pendings[_head];
        ++_head;
        ++done;
        _emiting = true;
        for (size_t i = 0; i < _set.size(); ++i) {
          Listener* v = _set[i];
          const Fn* f = find_handler(v, _key);
          if (f != nullptr) (*f)(args);
          if (v->once) _waits.push_back({WaitKind::kDel, v});
        }
        handle_waits();
        _emiting = false;
      }

      _overflow_warned = false;
      compact();
    }

    std::u16string _key;
    bool _emiting = false;
    bool _overflow_warned = false;
    size_t _head = 0;
    std::vector<Payloads> _pendings;
    std::vector<Listener*> _set;
    std::vector<Wait> _waits;
  };

  static std::u16string overflow_message(const std::u16string& key, size_t remaining) {
    return u"[NoEmitCallbacks::" + key + u"] " + number_to_string(static_cast<double>(kMaxPendingsPerFlush)) +
           u"+ jobs done in a single flush, " + number_to_string(static_cast<double>(remaining)) +
           u" still pending \u2014 possible broadcast loop? Remaining jobs will be dispatched on the "
           u"next emit.";
  }

  Pack& pack_for(const std::u16string& key) {
    for (Pack& pack : _packs) {
      if (pack.key() == key) return pack;
    }
    _packs.emplace_back(key);
    return _packs.back();
  }

  std::vector<Pack> _packs;
  std::vector<std::unique_ptr<Listener>> _owned;
};

using NoEmitCallbacks = NoEmitCallbacksT<Value>;

template <typename Payload>
class CallbacksT : public NoEmitCallbacksT<Payload> {
 public:
  void call(const std::u16string& key, const typename NoEmitCallbacksT<Payload>::Payloads& args) {
    this->emit(key, args);
  }
};

using Callbacks = CallbacksT<Value>;

}
