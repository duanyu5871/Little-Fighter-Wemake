#pragma once

#include <vector>

#include "lfw/defines/enum_entries.h"

namespace lfw {

namespace oid {

inline constexpr const char16_t* kTemplate = u"0";
inline constexpr const char16_t* kJulian = u"52";
inline constexpr const char16_t* kFirzen = u"51";
inline constexpr const char16_t* kLouisEX = u"50";
inline constexpr const char16_t* kBat = u"38";
inline constexpr const char16_t* kJustin = u"39";
inline constexpr const char16_t* kKnight = u"37";
inline constexpr const char16_t* kJan = u"36";
inline constexpr const char16_t* kMonk = u"35";
inline constexpr const char16_t* kSorcerer = u"34";
inline constexpr const char16_t* kJack = u"33";
inline constexpr const char16_t* kMark = u"32";
inline constexpr const char16_t* kHunter = u"31";
inline constexpr const char16_t* kBandit = u"30";
inline constexpr const char16_t* kDeep = u"1";
inline constexpr const char16_t* kJohn = u"2";
inline constexpr const char16_t* kHenry = u"4";
inline constexpr const char16_t* kRudolf = u"5";
inline constexpr const char16_t* kLouis = u"6";
inline constexpr const char16_t* kFiren = u"7";
inline constexpr const char16_t* kFreeze = u"8";
inline constexpr const char16_t* kDennis = u"9";
inline constexpr const char16_t* kWoody = u"10";
inline constexpr const char16_t* kDavis = u"11";
inline constexpr const char16_t* kWeapon0 = u"100";
inline constexpr const char16_t* kWeapon_Stick = u"100";
inline constexpr const char16_t* kWeapon2 = u"101";
inline constexpr const char16_t* kWeapon_Hoe = u"101";
inline constexpr const char16_t* kWeapon4 = u"120";
inline constexpr const char16_t* kWeapon_Knife = u"120";
inline constexpr const char16_t* kWeapon5 = u"121";
inline constexpr const char16_t* kWeapon_baseball = u"121";
inline constexpr const char16_t* kWeapon6 = u"122";
inline constexpr const char16_t* kWeapon_milk = u"122";
inline constexpr const char16_t* kWeapon1 = u"150";
inline constexpr const char16_t* kWeapon_Stone = u"150";
inline constexpr const char16_t* kWeapon3 = u"151";
inline constexpr const char16_t* kWeapon_WoodenBox = u"151";
inline constexpr const char16_t* kWeapon8 = u"123";
inline constexpr const char16_t* kWeapon_Beer = u"123";
inline constexpr const char16_t* kWeapon9 = u"124";
inline constexpr const char16_t* kWeapon_Boomerang = u"124";
inline constexpr const char16_t* kWeapon10 = u"217";
inline constexpr const char16_t* kWeapon_LouisArmourA = u"217";
inline constexpr const char16_t* kWeapon11 = u"218";
inline constexpr const char16_t* kWeapon_LouisArmourB = u"218";
inline constexpr const char16_t* kCriminal = u"300";
inline constexpr const char16_t* kJohnBall = u"200";
inline constexpr const char16_t* kHenryArrow1 = u"201";
inline constexpr const char16_t* kRudolfWeapon = u"202";
inline constexpr const char16_t* kDeepBall = u"203";
inline constexpr const char16_t* kHenryWind = u"204";
inline constexpr const char16_t* kDennisBall = u"205";
inline constexpr const char16_t* kWoodyBall = u"206";
inline constexpr const char16_t* kDavisBall = u"207";
inline constexpr const char16_t* kHenryArrow2 = u"208";
inline constexpr const char16_t* kFreezeBall = u"209";
inline constexpr const char16_t* kFirenBall = u"210";
inline constexpr const char16_t* kFirenFlame = u"211";
inline constexpr const char16_t* kFreezeColumn = u"212";
inline constexpr const char16_t* kWeapon7 = u"213";
inline constexpr const char16_t* kWeapon_IceSword = u"213";
inline constexpr const char16_t* kJohnBiscuit = u"214";
inline constexpr const char16_t* kDennisChase = u"215";
inline constexpr const char16_t* kJackBall = u"216";
inline constexpr const char16_t* kJanChaseh = u"219";
inline constexpr const char16_t* kJanChase = u"220";
inline constexpr const char16_t* kFirzenChasef = u"221";
inline constexpr const char16_t* kFirzenChasei = u"222";
inline constexpr const char16_t* kFirzenBall = u"223";
inline constexpr const char16_t* kBatBall = u"224";
inline constexpr const char16_t* kBatChase = u"225";
inline constexpr const char16_t* kJustinBall = u"226";
inline constexpr const char16_t* kJulianBall = u"228";
inline constexpr const char16_t* kJulianBall2 = u"229";
inline constexpr const char16_t* kEtc = u"998";
inline constexpr const char16_t* kBrokenWeapon = u"999";

}

inline const std::vector<EnumTextEntry>& oid_entries() {
  static const std::vector<EnumTextEntry> e = {
    {u"Template", oid::kTemplate},
    {u"Julian", oid::kJulian},
    {u"Firzen", oid::kFirzen},
    {u"LouisEX", oid::kLouisEX},
    {u"Bat", oid::kBat},
    {u"Justin", oid::kJustin},
    {u"Knight", oid::kKnight},
    {u"Jan", oid::kJan},
    {u"Monk", oid::kMonk},
    {u"Sorcerer", oid::kSorcerer},
    {u"Jack", oid::kJack},
    {u"Mark", oid::kMark},
    {u"Hunter", oid::kHunter},
    {u"Bandit", oid::kBandit},
    {u"Deep", oid::kDeep},
    {u"John", oid::kJohn},
    {u"Henry", oid::kHenry},
    {u"Rudolf", oid::kRudolf},
    {u"Louis", oid::kLouis},
    {u"Firen", oid::kFiren},
    {u"Freeze", oid::kFreeze},
    {u"Dennis", oid::kDennis},
    {u"Woody", oid::kWoody},
    {u"Davis", oid::kDavis},
    {u"Weapon0", oid::kWeapon0},
    {u"Weapon_Stick", oid::kWeapon_Stick},
    {u"Weapon2", oid::kWeapon2},
    {u"Weapon_Hoe", oid::kWeapon_Hoe},
    {u"Weapon4", oid::kWeapon4},
    {u"Weapon_Knife", oid::kWeapon_Knife},
    {u"Weapon5", oid::kWeapon5},
    {u"Weapon_baseball", oid::kWeapon_baseball},
    {u"Weapon6", oid::kWeapon6},
    {u"Weapon_milk", oid::kWeapon_milk},
    {u"Weapon1", oid::kWeapon1},
    {u"Weapon_Stone", oid::kWeapon_Stone},
    {u"Weapon3", oid::kWeapon3},
    {u"Weapon_WoodenBox", oid::kWeapon_WoodenBox},
    {u"Weapon8", oid::kWeapon8},
    {u"Weapon_Beer", oid::kWeapon_Beer},
    {u"Weapon9", oid::kWeapon9},
    {u"Weapon_Boomerang", oid::kWeapon_Boomerang},
    {u"Weapon10", oid::kWeapon10},
    {u"Weapon_LouisArmourA", oid::kWeapon_LouisArmourA},
    {u"Weapon11", oid::kWeapon11},
    {u"Weapon_LouisArmourB", oid::kWeapon_LouisArmourB},
    {u"Criminal", oid::kCriminal},
    {u"JohnBall", oid::kJohnBall},
    {u"HenryArrow1", oid::kHenryArrow1},
    {u"RudolfWeapon", oid::kRudolfWeapon},
    {u"DeepBall", oid::kDeepBall},
    {u"HenryWind", oid::kHenryWind},
    {u"DennisBall", oid::kDennisBall},
    {u"WoodyBall", oid::kWoodyBall},
    {u"DavisBall", oid::kDavisBall},
    {u"HenryArrow2", oid::kHenryArrow2},
    {u"FreezeBall", oid::kFreezeBall},
    {u"FirenBall", oid::kFirenBall},
    {u"FirenFlame", oid::kFirenFlame},
    {u"FreezeColumn", oid::kFreezeColumn},
    {u"Weapon7", oid::kWeapon7},
    {u"Weapon_IceSword", oid::kWeapon_IceSword},
    {u"JohnBiscuit", oid::kJohnBiscuit},
    {u"DennisChase", oid::kDennisChase},
    {u"JackBall", oid::kJackBall},
    {u"JanChaseh", oid::kJanChaseh},
    {u"JanChase", oid::kJanChase},
    {u"FirzenChasef", oid::kFirzenChasef},
    {u"FirzenChasei", oid::kFirzenChasei},
    {u"FirzenBall", oid::kFirzenBall},
    {u"BatBall", oid::kBatBall},
    {u"BatChase", oid::kBatChase},
    {u"JustinBall", oid::kJustinBall},
    {u"JulianBall", oid::kJulianBall},
    {u"JulianBall2", oid::kJulianBall2},
    {u"Etc", oid::kEtc},
    {u"BrokenWeapon", oid::kBrokenWeapon},
  };
  return e;
}

}
