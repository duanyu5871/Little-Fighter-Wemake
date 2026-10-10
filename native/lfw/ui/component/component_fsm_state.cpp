#include "lfw/ui/component/component_fsm_state.h"

#include "lfw/lfw.h"
#include "lfw/ui/component/ui_component.h"
#include "lfw/ui/uinode.h"
#include "lfw/world.h"

namespace lfw {
namespace ui {

ComponentFSMState::ComponentFSMState(UIComponent& owner)
    : owner(owner), lfw(owner.lfw()), world(owner.world()), node(owner.node) {}

}
}
