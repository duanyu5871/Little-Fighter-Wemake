#include "lfw/bot/dummy_enum.h"

#include <string>
#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {
namespace bot {
namespace {

const std::vector<EnumTextEntry> kDummyEnumEntries = {
    {u"None", dummy_enum::kNone},
    {u"LockAtMid_Stand", dummy_enum::kLockAtMid_Stand},
    {u"LockAtMid_Defend", dummy_enum::kLockAtMid_Defend},
    {u"LockAtMid_RowingWhenFalling", dummy_enum::kLockAtMid_RowingWhenFalling},
    {u"LockAtMid_JumpAndRowingWhenFalling", dummy_enum::kLockAtMid_JumpAndRowingWhenFalling},
    {u"AvoidEnemyAllTheTime", dummy_enum::kAvoidEnemyAllTheTime},
    {u"LockAtMid_dUa", dummy_enum::kLockAtMid_dUa},
    {u"LockAtMid_dUj", dummy_enum::kLockAtMid_dUj},
    {u"LockAtMid_dDa", dummy_enum::kLockAtMid_dDa},
    {u"LockAtMid_dDj", dummy_enum::kLockAtMid_dDj},
    {u"LockAtMid_dLa", dummy_enum::kLockAtMid_dLa},
    {u"LockAtMid_dLj", dummy_enum::kLockAtMid_dLj},
    {u"LockAtMid_dRa", dummy_enum::kLockAtMid_dRa},
    {u"LockAtMid_dRj", dummy_enum::kLockAtMid_dRj},
    {u"LockAtMid_dja", dummy_enum::kLockAtMid_dja},
    {u"LockAtMid_dUa_auto", dummy_enum::kLockAtMid_dUa_auto},
    {u"LockAtMid_dUj_auto", dummy_enum::kLockAtMid_dUj_auto},
    {u"LockAtMid_dDa_auto", dummy_enum::kLockAtMid_dDa_auto},
    {u"LockAtMid_dDj_auto", dummy_enum::kLockAtMid_dDj_auto},
    {u"LockAtMid_dLa_auto", dummy_enum::kLockAtMid_dLa_auto},
    {u"LockAtMid_dLj_auto", dummy_enum::kLockAtMid_dLj_auto},
    {u"LockAtMid_dRa_auto", dummy_enum::kLockAtMid_dRa_auto},
    {u"LockAtMid_dRj_auto", dummy_enum::kLockAtMid_dRj_auto},
    {u"LockAtMid_dja_auto", dummy_enum::kLockAtMid_dja_auto},
};

const std::vector<std::u16string> kDummyUpdaterIds = {
    dummy_enum::kNone,
    dummy_enum::kLockAtMid_Stand,
    dummy_enum::kLockAtMid_Defend,
    dummy_enum::kLockAtMid_RowingWhenFalling,
    dummy_enum::kLockAtMid_JumpAndRowingWhenFalling,
    dummy_enum::kAvoidEnemyAllTheTime,
    dummy_enum::kLockAtMid_dUa,
    dummy_enum::kLockAtMid_dUj,
    dummy_enum::kLockAtMid_dDa,
    dummy_enum::kLockAtMid_dDj,
    dummy_enum::kLockAtMid_dLa,
    dummy_enum::kLockAtMid_dLj,
    dummy_enum::kLockAtMid_dRa,
    dummy_enum::kLockAtMid_dRj,
    dummy_enum::kLockAtMid_dja,
    dummy_enum::kLockAtMid_dUa_auto,
    dummy_enum::kLockAtMid_dUj_auto,
    dummy_enum::kLockAtMid_dDa_auto,
    dummy_enum::kLockAtMid_dDj_auto,
    dummy_enum::kLockAtMid_dLj_auto,
    dummy_enum::kLockAtMid_dRa_auto,
    dummy_enum::kLockAtMid_dRj_auto,
    dummy_enum::kLockAtMid_dja_auto,
};

}

const std::vector<EnumTextEntry>& dummy_enum_entries() { return kDummyEnumEntries; }

const std::vector<std::u16string>& dummy_updater_ids() { return kDummyUpdaterIds; }

}
}
