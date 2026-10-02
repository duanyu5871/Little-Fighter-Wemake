#pragma once

#include "lfw/collision/collision.h"

namespace lfw {
namespace collision {

void handle_super_punch_me(Collision& c);
void handle_weapon_picked(Collision& c);
void handle_stiffness(Collision& c);
void handle_body_goto(Collision& c);
void handle_rest(Collision& c);
void handle_itr_kind_magic_flute(Collision& c);

}
}
