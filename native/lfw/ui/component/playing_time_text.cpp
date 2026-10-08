#include "lfw/ui/component/playing_time_text.h"

#include "lfw/core/js_string.h"
#include "lfw/stage/stage.h"
#include "lfw/ui/uinode.h"
#include "lfw/utils/math/base.h"
#include "lfw/world.h"

namespace lfw {
namespace ui {

const ClazzTag* PlayingTimeText::class_tag() {
  static const ClazzTag tag{UIComponent::class_tag()};
  return &tag;
}

const std::vector<std::u16string>& PlayingTimeText::TAGS() {
  static const std::vector<std::u16string> tags{u"PlayingTimeText"};
  return tags;
}

void PlayingTimeText::on_show() {
  node.set_text(get_txt());
}

std::u16string PlayingTimeText::get_txt() {
  const stage::Stage* const s = world().stage();
  const double duration = floor((s != nullptr ? s->time() : 0.0) * 16.6666);
  const double sec = floor(duration / 1000) - floor(duration / (60 * 1000)) * 60;
  const double min = floor(duration / (60 * 1000)) - floor(duration / (60 * 60 * 1000)) * 60;
  const double hour =
      floor(duration / (60 * 60 * 1000)) - floor(duration / (60 * 60 * 1000) / 60) * 60;
  std::u16string ret;
  if (hour != 0) ret += number_to_string(hour) + u":";
  if (min > 9 || hour == 0) {
    ret += number_to_string(min) + u":";
  } else {
    ret += u"0" + number_to_string(min) + u":";
  }
  if (sec > 9) ret += number_to_string(sec);
  else ret += u"0" + number_to_string(sec);
  return ret;
}

}
}
