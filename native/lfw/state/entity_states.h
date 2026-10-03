#pragma once

#include "lfw/state/states.h"

namespace lfw {
namespace state {

// Registry of every ported state, mirroring `src/LFW/state/ENTITY_STATES.ts`.
States& entity_states();

}
}
