#pragma once

#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

#include "lfw/core/value.h"

namespace lfw {

class CondMaker;

using CondTermFormatter =
    std::function<std::u16string(const Value&, const std::u16string&, const Value&)>;
using CondEdit = std::function<CondMaker*(CondMaker&)>;

struct CondPart {
  bool is_maker = false;
  std::u16string text;
  CondMaker* maker = nullptr;
};

class CondMaker {
 public:
  enum Allow : unsigned {
    kAllowTerm = 0,
    kAllowNone = 1,
    kAllowOr = 2,
    kAllowAnd = 4,
    kAllowNot = 8,
  };

  CondMaker();
  CondMaker(const CondMaker&) = delete;
  CondMaker& operator=(const CondMaker&) = delete;

  bool ok() const { return _error.empty(); }
  const std::u16string& error() const { return _error; }

  CondMaker& quote_strings(bool on = true, char16_t quote = u'"');
  CondMaker& term_format(CondTermFormatter fn);

  CondMaker& add(CondEdit fn);
  CondMaker& add(const Value& v1, const std::u16string& op, const Value& v2);

  CondMaker& wrap(CondEdit fn);
  CondMaker& not_(CondEdit fn);
  CondMaker& or_(CondEdit fn);
  CondMaker& or_(const Value& v1, const std::u16string& op, const Value& v2);
  CondMaker& and_(CondEdit fn);
  CondMaker& and_(const Value& v1, const std::u16string& op, const Value& v2);

  CondMaker& one_of(const Value& v1, const std::vector<Value>& v2);
  CondMaker& and_one_of(const Value& v1, const std::vector<Value>& v2);
  CondMaker& or_one_of(const Value& v1, const std::vector<Value>& v2);
  CondMaker& not_in(const Value& v1, const std::vector<Value>& v2);
  CondMaker& and_not_in(const Value& v1, const std::vector<Value>& v2);
  CondMaker& or_not_in(const Value& v1, const std::vector<Value>& v2);

  std::u16string done() const;

 private:
  void fail(const char16_t* func, const std::u16string& msg);
  void opok(const char16_t* func, unsigned allow);
  bool assert_value(const Value& v, const char16_t* where);
  bool assert_op(const std::u16string& op, const char16_t* where);
  CondMaker* make_child();
  void push_text(std::u16string t);
  void push_maker(CondMaker* m);
  std::u16string quote_text(const std::u16string& v) const;
  unsigned last_kind() const;
  static std::u16string part_text(const CondPart& p);

  bool _quote_on = false;
  char16_t _quote_char = u'"';
  std::vector<CondPart> _parts;
  CondTermFormatter _term;
  std::vector<std::unique_ptr<CondMaker>> _children;
  std::u16string _error;
};

}
