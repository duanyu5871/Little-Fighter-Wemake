#pragma once

#include "lfw/dat_translator/bots/bot_maker.h"

namespace lfw {
namespace dat_translator {
namespace bots {

BotMaker make_bot_data_bat();
BotMaker make_bot_data_davis();
BotMaker make_bot_data_hunter();
BotMaker make_bot_data_jack();
BotMaker make_bot_data_jan();
BotMaker make_bot_data_justin();
BotMaker make_bot_data_knight();
BotMaker make_bot_data_louis();
BotMaker make_bot_data_mark();
BotMaker make_bot_data_monk();
BotMaker make_bot_data_sorcerer();

void register_all_bots();

}
}
}
