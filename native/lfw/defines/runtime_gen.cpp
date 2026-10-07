#include "lfw/defines/runtime_gen.h"

#include <optional>

#include "lfw/core/json5.h"

namespace lfw {
namespace {

const char16_t kJson0[] = u"[1,2,3,4,8,9,10,16,17,18,32,33,34,60,61,62,63,128]";
const char16_t kJson1[] = u"{\"0\":\"\",\"1\":\"\",\"2000\":\"\",\"10000\":\"\"}";
const char16_t kJson2[] = u"{\"L\":\"R\",\"R\":\"L\",\"U\":\"D\",\"D\":\"U\"}";
const char16_t kJson3[] = u"75";
const char16_t kJson4[] = u"76";
const char16_t kJson5[] = u"120";
const char16_t kJson6[] = u"120";
const char16_t kJson7[] = u"100";
const char16_t kJson8[] = u"100";
const char16_t kJson9[] = u"250";
const char16_t kJson10[] = u"250";
const char16_t kJson11[] = u"1";
const char16_t kJson12[] = u"1";
const char16_t kJson13[] = u"3";
const char16_t kJson14[] = u"200";
const char16_t kJson15[] = u"{\"ResetGPL\":\"reset_gpl\",\"UpdateRandom\":\"update_random\",\"StartGame\":\"start_game\",\"SwitchStage\":\"switch_stage\",\"SwitchStageR\":\"switch_stage_r\",\"SwitchBackground\":\"switch_background\",\"SwitchBackgroundR\":\"switch_background_r\",\"ResetBackground\":\"reset_background\"}";
const char16_t kJson16[] = u"{\"Spark\":\"data/spark.obj.json5\"}";
const char16_t kJson17[] = u"{\"RFACE\":\"!sprite/RFACE@4x.png\",\"CHARACTER_THUMB\":\"sprite/CHARACTER_THUMB.png\"}";
const char16_t kJson18[] = u"{\"Cancel\":\"cancel\",\"End\":\"end\",\"Join\":\"join\",\"Ok\":\"ok\",\"Pass\":\"pass\"}";
const char16_t kJson19[] = u"550";
const char16_t kJson20[] = u"794";
const char16_t kJson21[] = u"{\"LF2_NET\":{\"keys\":\"lf2.net\",\"gkeys\":\"UUDDLRLRdada\",\"sound\":\"data/m_pass.wav.mp3\"},\"HERO_FT\":{\"keys\":\"herofighter.com\",\"gkeys\":\"UUDDLRLRjaja\",\"sound\":\"data/m_end.wav.mp3\"},\"GIM_INK\":{\"keys\":\"gim.ink\",\"gkeys\":\"UUDDLRLRdjdj\",\"sound\":\"data/093_r.wav.mp3\"}}";
const char16_t kJson22[] = u"24";
const char16_t kJson23[] = u"576";
const char16_t kJson24[] = u"\"danmu_uid\"";
const char16_t kJson25[] = u"20";
const char16_t kJson26[] = u"0.1";
const char16_t kJson27[] = u"1.5";
const char16_t kJson28[] = u"2";
const char16_t kJson29[] = u"32";
const char16_t kJson30[] = u"100";
const char16_t kJson31[] = u"40";
const char16_t kJson32[] = u"0.5";
const char16_t kJson33[] = u"200";
const char16_t kJson34[] = u"3.5";
const char16_t kJson35[] = u"60";
const char16_t kJson36[] = u"{\"1\":\"easy\",\"2\":\"normal\",\"3\":\"difficult\",\"4\":\"crazy\"}";
const char16_t kJson37[] = u"10000";
const char16_t kJson38[] = u"450";
const char16_t kJson39[] = u"794";
const char16_t kJson40[] = u"{\"id\":\"auto\"}";
const char16_t kJson41[] = u"{\"id\":\"auto\",\"facing\":2}";
const char16_t kJson42[] = u"{\"id\":\"gone\"}";
const char16_t kJson43[] = u"{\"id\":\"self\"}";
const char16_t kJson44[] = u"{\"type\":\"background\",\"layers\":[],\"id\":\"?\",\"base\":{\"name\":\"Random\",\"shadow\":\"\",\"height\":550,\"left\":0,\"right\":794,\"far\":-468,\"near\":-216,\"group\":[],\"shadow_w\":0,\"shadow_h\":0}}";
const char16_t kJson45[] = u"{\"ARROWUP\":\"\u2191\",\"ARROWDOWN\":\"\u2193\",\"ARROWLEFT\":\"\u2190\",\"ARROWRIGHT\":\"\u2192\",\"DELETE\":\"DEL\",\"PAGEDOWN\":\"PG\u2193\",\"PAGEUP\":\"PG\u2191\"}";
const char16_t kJson46[] = u"104";
const char16_t kJson47[] = u"{\"StagePass\":\"data/m_pass.wav.mp3\",\"BattleEnd\":\"data/m_end.wav.mp3\"}";
const char16_t kJson48[] = u"{\"Independent\":\"\",\"Team_1\":\"1\",\"Team_2\":\"2\",\"Team_3\":\"3\",\"Team_4\":\"4\",\"Team_5\":\"5\",\"Team_6\":\"6\",\"Team_7\":\"7\",\"Team_8\":\"8\",\"Max\":\"8\"}";
const char16_t kJson49[] = u"{\"1\":{\"i18n\":\"Team_1\",\"txt_color\":\"#4f9bff\",\"txt_outline_color\":\"#001e46\",\"outline_color\":\"#4f9bff\"},\"2\":{\"i18n\":\"Team_2\",\"txt_color\":\"#ff4f4f\",\"txt_outline_color\":\"#460000\",\"outline_color\":\"#ff4f4f\"},\"3\":{\"i18n\":\"Team_3\",\"txt_color\":\"#3cad0f\",\"txt_outline_color\":\"#154103\",\"outline_color\":\"#3cad0f\"},\"4\":{\"i18n\":\"Team_4\",\"txt_color\":\"#ffd34c\",\"txt_outline_color\":\"#573100\",\"outline_color\":\"#ffd34c\"},\"5\":{\"i18n\":\"Team_5\",\"txt_color\":\"#ff00bf\",\"txt_outline_color\":\"#2b0020\",\"outline_color\":\"#ff5cd6\"},\"6\":{\"i18n\":\"Team_6\",\"txt_color\":\"#00ffff\",\"txt_outline_color\":\"#003131\",\"outline_color\":\"#00ffff\"},\"7\":{\"i18n\":\"Team_7\",\"txt_color\":\"#000000\",\"txt_outline_color\":\"#ff0000\",\"outline_color\":\"#300000\"},\"8\":{\"i18n\":\"Team_8\",\"txt_color\":\"#000000\",\"txt_outline_color\":\"#ffffff\",\"outline_color\":\"#ffffff\"},\"\":{\"i18n\":\"Independent\",\"txt_color\":\"#ffffff\",\"txt_outline_color\":\"#000000\"}}";
const char16_t kJson50[] = u"[\"\",\"1\",\"2\",\"3\",\"4\"]";
const char16_t kJson51[] = u"{\"type\":\"background\",\"layers\":[],\"id\":\"VOID_BG\",\"base\":{\"name\":\"VOID_BG\",\"shadow\":\"\",\"height\":550,\"left\":0,\"right\":1588,\"far\":-40,\"near\":-0,\"group\":[],\"zoom_x\":0.5,\"zoom_y\":0.5,\"zoom_z\":0.5,\"shadow_w\":0,\"shadow_h\":0}}";
const char16_t kJson52[] = u"{\"bg\":\"VOID_BG\",\"id\":\"VOID_STAGE\",\"name\":\"VOID_STAGE\",\"phases\":[],\"group\":[\"Dev\"]}";
const char16_t kJson53[] = u"0.74";
const char16_t kJson54[] = u"0.6";
const char16_t kJson55[] = u"1.1";
const char16_t kJson56[] = u"1";
const char16_t kJson57[] = u"0.74";
const char16_t kJson58[] = u"0.9";
const char16_t kJson59[] = u"{\"0\":99,\"1\":99,\"2\":2,\"3\":99,\"4\":2,\"5\":2}";
const char16_t kJson60[] = u"{\"0\":2,\"1\":2,\"2\":2,\"3\":2,\"4\":1,\"5\":1}";
const char16_t kJson61[] = u"{\"0\":99,\"1\":99,\"2\":99,\"3\":99,\"4\":99,\"5\":99}";
const char16_t kJson62[] = u"{\"0\":0.5,\"1\":0.5,\"2\":0.75,\"3\":0.5,\"4\":0.75,\"5\":0.75}";
const char16_t kJson63[] = u"{\"0\":0.5,\"1\":0.2,\"2\":0.3,\"3\":0.2,\"4\":0.45,\"5\":0.45}";
const char16_t kJson64[] = u"{\"0\":0.5,\"1\":0.5,\"2\":0.75,\"3\":0.5,\"4\":0.75,\"5\":0.75}";
const char16_t kJson65[] = u"{\"0\":99,\"1\":99,\"2\":1,\"3\":99,\"4\":4.5,\"5\":4.5}";
const char16_t kJson66[] = u"{\"0\":99,\"1\":99,\"2\":1,\"3\":99,\"4\":99,\"5\":99}";
const char16_t kJson67[] = u"{\"0\":99,\"1\":99,\"2\":1,\"3\":99,\"4\":99,\"5\":99}";
const char16_t kJson68[] = u"{\"1\":{\"L\":\"a\",\"R\":\"d\",\"U\":\"w\",\"D\":\"s\",\"a\":\"j\",\"j\":\"k\",\"d\":\"l\"},\"2\":{\"L\":\"arrowleft\",\"R\":\"arrowright\",\"U\":\"arrowup\",\"D\":\"arrowdown\",\"a\":\"0\",\"j\":\".\",\"d\":\"enter\"},\"3\":{\"L\":\"\",\"R\":\"\",\"U\":\"\",\"D\":\"\",\"a\":\"\",\"j\":\"\",\"d\":\"\"},\"4\":{\"L\":\"\",\"R\":\"\",\"U\":\"\",\"D\":\"\",\"a\":\"\",\"j\":\"\",\"d\":\"\"},\"_\":{\"L\":\"_L\",\"R\":\"_R\",\"U\":\"_U\",\"D\":\"_D\",\"a\":\"_a\",\"j\":\"_j\",\"d\":\"_d\"}}";
const char16_t kJson69[] = u"{\"1\":\"\",\"2\":\"\",\"3\":\"\",\"4\":\"\"}";
const char16_t kJson70[] = u"[1,2,3,4]";
const char16_t kJson71[] = u"{\"1\":\"Easy\",\"2\":\"Normal\",\"3\":\"Difficult\",\"4\":\"Crazy!\"}";
const char16_t kJson72[] = u"{\"id\":\"\",\"name\":\"EMPTY_FRAME_INFO\",\"pic\":{\"tex\":\"\",\"x\":0,\"y\":0,\"w\":0,\"h\":0},\"width\":0,\"height\":0,\"state\":NaN,\"wait\":0,\"next\":{\"id\":\"auto\"},\"centerx\":0,\"centery\":0}";
const char16_t kJson73[] = u"{\"4\":3,\"8\":0,\"16\":1,\"32\":2}";
const char16_t kJson74[] = u"{\"id\":\"gone\",\"name\":\"GONE_FRAME_INFO\",\"pic\":{\"tex\":\"\",\"x\":0,\"y\":0,\"w\":0,\"h\":0},\"width\":0,\"height\":0,\"state\":NaN,\"wait\":0,\"next\":{\"id\":\"gone\"},\"centerx\":0,\"centery\":0,\"no_shadow\":1}";
const char16_t kJson75[] = u"{\"1\":\"\u654c\u4eba\",\"2\":\"\u961f\u53cb\",\"3\":\"\u5224\u5b9a\u961f\u53cb\u4e0e\u961f\u53cb\",\"4\":\"Ohters\",\"8\":\"Fighter\",\"9\":\"EnemyFighter\",\"10\":\"AllyFighter\",\"16\":\"Weapon\",\"17\":\"EnemyWeapon\",\"18\":\"AllyWeapon\",\"32\":\"Ball\",\"33\":\"EnemyBall\",\"34\":\"AllyBall\",\"60\":\"\u5168\u7c7b\u578b\",\"61\":\"\u5224\u5b9a\u5168\u7c7b\u578b\u654c\u4eba\",\"62\":\"\u5224\u5b9a\u5168\u7c7b\u578b\u961f\u53cb\",\"63\":\"\u5224\u5b9a\u5168\u7c7b\u578b\u654c\u4eba\u4e0e\u961f\u53cb\",\"128\":\"Dead\"}";
const char16_t kJson76[] = u"{\"1\":\"Enemy\",\"2\":\"Ally\",\"3\":\"Both\",\"4\":\"Ohters\",\"8\":\"Fighter\",\"9\":\"EnemyFighter\",\"10\":\"AllyFighter\",\"16\":\"Weapon\",\"17\":\"EnemyWeapon\",\"18\":\"AllyWeapon\",\"32\":\"Ball\",\"33\":\"EnemyBall\",\"34\":\"AllyBall\",\"60\":\"AllType\",\"61\":\"AllEnemy\",\"62\":\"AllAlly\",\"63\":\"AllBoth\",\"128\":\"Dead\"}";
const char16_t kJson77[] = u"{\"1\":\"\",\"2\":\"\",\"3\":\"\",\"4\":\"\",\"8\":\"\",\"9\":\"\",\"10\":\"\",\"16\":\"\",\"17\":\"\",\"18\":\"\",\"32\":\"\",\"33\":\"\",\"34\":\"\",\"60\":\"\",\"61\":\"\",\"62\":\"\",\"63\":\"\",\"128\":\"\"}";
const char16_t kJson78[] = u"1999";
const char16_t kJson79[] = u"1000";
const char16_t kJson80[] = u"{\"0\":\"None\",\"1\":\"Bearer\",\"2\":\"Holded\",\"3\":\"Drop\"}";

struct Raw {
  const char16_t* name;
  const char16_t* json;
  bool top;
};

const Raw kRaw[] = {
    {u"ALL_HIT_FLAG", kJson0, true},
    {u"BdyKindDescriptions", kJson1, true},
    {u"CONFLICTS_KEY_MAP", kJson2, true},
    {u"Defines.AI_COME_RANGE_IN_X", kJson3, false},
    {u"Defines.AI_COME_RANGE_IN_Z", kJson4, false},
    {u"Defines.AI_COME_RANGE_OUT_X", kJson5, false},
    {u"Defines.AI_COME_RANGE_OUT_Z", kJson6, false},
    {u"Defines.AI_FOLLOWING_RANGE_IN_X", kJson7, false},
    {u"Defines.AI_FOLLOWING_RANGE_IN_Z", kJson8, false},
    {u"Defines.AI_FOLLOWING_RANGE_OUT_X", kJson9, false},
    {u"Defines.AI_FOLLOWING_RANGE_OUT_Z", kJson10, false},
    {u"Defines.AI_MAX_AVOIDING_ENEMIES", kJson11, false},
    {u"Defines.AI_MAX_CHASINGS_ENEMIES", kJson12, false},
    {u"Defines.AI_MAX_DEFENDS_ENEMIES", kJson13, false},
    {u"Defines.AI_STAY_CHASING_RANGE", kJson14, false},
    {u"Defines.BuiltIn_Broadcast", kJson15, false},
    {u"Defines.BuiltIn_Dats", kJson16, false},
    {u"Defines.BuiltIn_Imgs", kJson17, false},
    {u"Defines.BuiltIn_Sounds", kJson18, false},
    {u"Defines.CLASSIC_SCREEN_HEIGHT", kJson19, false},
    {u"Defines.CLASSIC_SCREEN_WIDTH", kJson20, false},
    {u"Defines.CheatInfos", kJson21, false},
    {u"Defines.DAFUALT_QUBE_LENGTH", kJson22, false},
    {u"Defines.DAFUALT_QUBE_LENGTH_POW2", kJson23, false},
    {u"Defines.DANMU_UID_MARK", kJson24, false},
    {u"Defines.DATA_VERSION", kJson25, false},
    {u"Defines.DEFAULT_ARMOR_INJURY_RATIO", kJson26, false},
    {u"Defines.DEFAULT_ARMOR_MOTIONLESS_RATIO", kJson27, false},
    {u"Defines.DEFAULT_ARMOR_SHAKING_RATIO", kJson28, false},
    {u"Defines.DEFAULT_BREAK_DEFEND_VALUE", kJson29, false},
    {u"Defines.DEFAULT_FALL_VALUE_CRITICAL", kJson30, false},
    {u"Defines.DEFAULT_FALL_VALUE_DIZZY", kJson31, false},
    {u"Defines.DEFAULT_FIREN_FLAME_SPEED_Z", kJson32, false},
    {u"Defines.DEFAULT_FORCE_BREAK_DEFEND_VALUE", kJson33, false},
    {u"Defines.DEFAULT_OPOINT_SPEED_Z", kJson34, false},
    {u"Defines.DEFAULT_TOUGHNESS_RESTING_MAX", kJson35, false},
    {u"Defines.DifficultyLabels", kJson36, false},
    {u"Defines.MAX_AI_DESIRE", kJson37, false},
    {u"Defines.MODERN_SCREEN_HEIGHT", kJson38, false},
    {u"Defines.MODERN_SCREEN_WIDTH", kJson39, false},
    {u"Defines.NEXT_FRAME_AUTO", kJson40, false},
    {u"Defines.NEXT_FRAME_AUTO_BACKWARD", kJson41, false},
    {u"Defines.NEXT_FRAME_GONE", kJson42, false},
    {u"Defines.NEXT_FRAME_SELF", kJson43, false},
    {u"Defines.RANDOM_BG", kJson44, false},
    {u"Defines.SHORT_KEY_CODES", kJson45, false},
    {u"Defines.STATE_HEAL_SELF_HP", kJson46, false},
    {u"Defines.Sounds", kJson47, false},
    {u"Defines.TeamEnum", kJson48, false},
    {u"Defines.TeamInfoMap", kJson49, false},
    {u"Defines.Teams", kJson50, false},
    {u"Defines.VOID_BG", kJson51, false},
    {u"Defines.VOID_STAGE", kJson52, false},
    {u"Defines.WEAPON_WEIGHT_ARROW", kJson53, false},
    {u"Defines.WEAPON_WEIGHT_BASEBALL", kJson54, false},
    {u"Defines.WEAPON_WEIGHT_HEAVY", kJson55, false},
    {u"Defines.WEAPON_WEIGHT_HOE", kJson56, false},
    {u"Defines.WEAPON_WEIGHT_LIGHT", kJson57, false},
    {u"Defines.WEAPON_WEIGHT_NOMRAL", kJson58, false},
    {u"Defines.WT_BOUNCE_MIN_X", kJson59, false},
    {u"Defines.WT_BOUNCE_MIN_Y", kJson60, false},
    {u"Defines.WT_BOUNCE_MIN_Z", kJson61, false},
    {u"Defines.WT_BOUNCE_X", kJson62, false},
    {u"Defines.WT_BOUNCE_Y", kJson63, false},
    {u"Defines.WT_BOUNCE_Z", kJson64, false},
    {u"Defines.WT_FAST_X", kJson65, false},
    {u"Defines.WT_FAST_Y", kJson66, false},
    {u"Defines.WT_FAST_Z", kJson67, false},
    {u"Defines.default_keys_map", kJson68, false},
    {u"DifficultyDescriptions", kJson69, true},
    {u"DifficultyList", kJson70, true},
    {u"DifficultyNames", kJson71, true},
    {u"EMPTY_FRAME_INFO", kJson72, true},
    {u"ENTITY_PRIORITY_MAP", kJson73, true},
    {u"GONE_FRAME_INFO", kJson74, true},
    {u"HIT_FLAG_DESC_MAP", kJson75, true},
    {u"HIT_FLAG_NAME_MAP", kJson76, true},
    {u"HitFlagDescriptions", kJson77, true},
    {u"OLD_BDY_KIND_GOTO_MAX", kJson78, true},
    {u"OLD_BDY_KIND_GOTO_MIN", kJson79, true},
    {u"WpointKindDescriptions", kJson80, true},
};

std::vector<DefinesRuntimeEntry> build() {
  std::vector<DefinesRuntimeEntry> out;
  out.reserve(sizeof(kRaw) / sizeof(kRaw[0]));
  for (const Raw& r : kRaw) out.push_back({r.name, r.json, r.top});
  return out;
}

}

const std::vector<DefinesRuntimeEntry>& defines_runtime_entries() {
  static const std::vector<DefinesRuntimeEntry> kEntries = build();
  return kEntries;
}

const char16_t kNew0[] = u"{}";
const char16_t kNew1[] = u"{\"width\":0,\"height\":0,\"x\":0,\"y\":0,\"z\":0,\"w\":0,\"h\":0}";
const char16_t kNew2[] = u"{\"type\":\"background\",\"id\":\"\",\"base\":{},\"layers\":[]}";
const char16_t kNew3[] = u"{\"type\":3,\"toughness\":2}";
const char16_t kNew4[] = u"{\"kind\":0}";
const char16_t kNew5[] = u"{\"x\":0,\"y\":0}";
const char16_t kNew6[] = u"{\"flag\":0,\"lost\":0}";
const char16_t kNew7[] = u"{}";
const char16_t kNew8[] = u"{\"id\":\"\",\"type\":\"\",\"file\":\"\"}";
const char16_t kNew9[] = u"{}";
const char16_t kNew10[] = u"{}";
const char16_t kNew11[] = u"{\"id\":\"\",\"type\":4,\"frames\":{},\"base\":{\"name\":\"\",\"type\":4}}";
const char16_t kNew12[] = u"{\"name\":\"\",\"type\":4}";
const char16_t kNew13[] = u"{}";
const char16_t kNew14[] = u"{\"id\":\"\",\"name\":\"\",\"state\":0,\"wait\":0,\"next\":{\"id\":\"\"},\"centerx\":0,\"centery\":0,\"width\":0,\"height\":0}";
const char16_t kNew15[] = u"{\"id\":\"\"}";
const char16_t kNew16[] = u"{\"tex\":\"\",\"x\":0,\"y\":0,\"w\":0,\"h\":0}";
const char16_t kNew17[] = u"{\"kind\":0}";
const char16_t kNew18[] = u"{\"id\":\"\",\"path\":\"\"}";
const char16_t kNew19[] = u"{}";
const char16_t kNew20[] = u"{\"kind\":1,\"x\":0,\"y\":0,\"oid\":\"\",\"action\":{\"id\":\"\"}}";
const char16_t kNew21[] = u"{\"type\":0}";
const char16_t kNew22[] = u"{\"id\":\"\",\"path\":\"\"}";
const char16_t kNew23[] = u"{\"path\":\"\"}";
const char16_t kNew24[] = u"{}";
const char16_t kNew25[] = u"{}";
const char16_t kNew26[] = u"{}";
const char16_t kNew27[] = u"{\"type\":0,\"x1\":0,\"x2\":0,\"z1\":0,\"z2\":0,\"h1\":0,\"h2\":0}";
const char16_t kNew28[] = u"{\"kind\":0,\"x\":0,\"y\":0,\"z\":0,\"weaponact\":\"\"}";

Value bg_info_new() {
  const Json5Result r = json5_parse(kNew0);
  return r.ok ? r.value : Value();
}

Value bg_layer_info_new() {
  const Json5Result r = json5_parse(kNew1);
  return r.ok ? r.value : Value();
}

Value bg_data_new() {
  const Json5Result r = json5_parse(kNew2);
  return r.ok ? r.value : Value();
}

Value armor_Info_new() {
  const Json5Result r = json5_parse(kNew3);
  return r.ok ? r.value : Value();
}

Value bdy_info_new() {
  const Json5Result r = json5_parse(kNew4);
  return r.ok ? r.value : Value();
}

Value bpoint_info_new() {
  const Json5Result r = json5_parse(kNew5);
  return r.ok ? r.value : Value();
}

Value chase_info_new() {
  const Json5Result r = json5_parse(kNew6);
  return r.ok ? r.value : Value();
}

Value cpoint_new() {
  const Json5Result r = json5_parse(kNew7);
  return r.ok ? r.value : Value();
}

Value dat_index_new() {
  const Json5Result r = json5_parse(kNew8);
  return r.ok ? r.value : Value();
}

Value dialog_info_new() {
  const Json5Result r = json5_parse(kNew9);
  return r.ok ? r.value : Value();
}

Value drink_info_new() {
  const Json5Result r = json5_parse(kNew10);
  return r.ok ? r.value : Value();
}

Value entity_data_new() {
  const Json5Result r = json5_parse(kNew11);
  return r.ok ? r.value : Value();
}

Value entity_info_new() {
  const Json5Result r = json5_parse(kNew12);
  return r.ok ? r.value : Value();
}

Value frame_indexes_new() {
  const Json5Result r = json5_parse(kNew13);
  return r.ok ? r.value : Value();
}

Value frame_info_new() {
  const Json5Result r = json5_parse(kNew14);
  return r.ok ? r.value : Value();
}

Value frame_model_new() {
  const Json5Result r = json5_parse(kNew15);
  return r.ok ? r.value : Value();
}

Value frame_pic_new() {
  const Json5Result r = json5_parse(kNew16);
  return r.ok ? r.value : Value();
}

Value itr_info_new() {
  const Json5Result r = json5_parse(kNew17);
  return r.ok ? r.value : Value();
}

Value model_info_new() {
  const Json5Result r = json5_parse(kNew18);
  return r.ok ? r.value : Value();
}

Value next_frame_new() {
  const Json5Result r = json5_parse(kNew19);
  return r.ok ? r.value : Value();
}

Value opoint_info_new() {
  const Json5Result r = json5_parse(kNew20);
  return r.ok ? r.value : Value();
}

Value opoint_multi_new() {
  const Json5Result r = json5_parse(kNew21);
  return r.ok ? r.value : Value();
}

Value picture_info_new() {
  const Json5Result r = json5_parse(kNew22);
  return r.ok ? r.value : Value();
}

Value sound_play_info_new() {
  const Json5Result r = json5_parse(kNew23);
  return r.ok ? r.value : Value();
}

Value stage_info_new() {
  const Json5Result r = json5_parse(kNew24);
  return r.ok ? r.value : Value();
}

Value stage_object_info_new() {
  const Json5Result r = json5_parse(kNew25);
  return r.ok ? r.value : Value();
}

Value stage_phase_info_new() {
  const Json5Result r = json5_parse(kNew26);
  return r.ok ? r.value : Value();
}

Value terrain_info_new() {
  const Json5Result r = json5_parse(kNew27);
  return r.ok ? r.value : Value();
}

Value wpoint_info_new() {
  const Json5Result r = json5_parse(kNew28);
  return r.ok ? r.value : Value();
}

}
