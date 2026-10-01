#pragma once

#include <optional>
#include <string>

#include "lfw/core/value.h"

namespace lfw {
namespace dat_translator {

void make_fb_bat_chase_start(Value& frame);
void make_fb_bat_chase(Value& frame);
void make_fb_boomerang(Value& frame);
void make_fb_chasing_same_enemy(Value& frame, const std::u16string& oid);
void make_fb_dennis_chase(Value& frame);
void make_fb_firzen_disater_start(Value& frame, std::optional<Value> x, std::optional<Value> y);
void make_fb_firzen_volcano_start(Value& frame, std::optional<Value> x, std::optional<Value> y);
void make_fb_jan_angle_blessing(Value& frame);
void make_fb_jan_chase_start(Value& frame, std::optional<Value> x, std::optional<Value> y);
void make_fb_jan_chaseh_start(Value& frame, std::optional<Value> x, std::optional<Value> y);
void make_fb_john_chase_leaving(Value& frame);
void make_fb_john_chase(Value& frame);
void make_fb_julian_ball_start(Value& frame);
void make_fb_julian_ball(Value& frame);

void make_frame_behavior(Value& frame, const std::u16string& oid);

}

}
