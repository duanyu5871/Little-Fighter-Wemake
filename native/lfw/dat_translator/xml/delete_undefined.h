#pragma once

#include "lfw/dat_translator/helpers.h"

namespace lfw {
namespace dat_translator {
namespace xml {

// `src/LFW/dat_translator/xml/delete_undefined.ts` 的实现早前已并进
// `dat_translator/helpers.*`（`Value& delete_undefined(Value&)`），这里只把名字转出：
// TS 里它属于 xml 目录，端口沿用同一实现以免两份漂移。
using dat_translator::delete_undefined;

}
}
}
