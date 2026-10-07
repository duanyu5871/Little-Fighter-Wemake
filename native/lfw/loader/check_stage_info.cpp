#include "lfw/loader/check_stage_info.h"

#include "lfw/defines/schemas_gen.h"
#include "lfw/utils/schema/validate_schema.h"

namespace lfw {
namespace loader {

bool check_stage_info(const Value& info, std::vector<std::u16string>* errors) {
  schema::SchemaValidator v;
  const bool result = v.validate(info, schema_i_stage_info());
  if (errors != nullptr) errors->insert(errors->end(), v.errors().begin(), v.errors().end());
  return result;
}

bool check_phase_info(const Value& stage, const Value& info, int idx,
                      std::vector<std::u16string>* errors) {
  // TS 的 `stage` / `idx` 只是签名的一部分（函数体没用到）——端口保留形参不落地。
  (void)stage;
  (void)idx;
  std::vector<std::u16string> local;
  std::vector<std::u16string>& sink = errors != nullptr ? *errors : local;
  schema::SchemaValidator v;
  const bool result = v.validate(info, schema_i_stage_phase_info());
  sink.insert(sink.end(), v.errors().begin(), v.errors().end());
  return result;
}

}
}
