#pragma once

#include "lfw/dat_translator/bots/bot_maker.h"

namespace lfw {
namespace dat_translator {
namespace bots {

BotMaker make_bot_data_bat();
BotMaker make_bot_data_hunter();
BotMaker make_bot_data_jan();
BotMaker make_bot_data_knight();
BotMaker make_bot_data_monk();

void register_all_bots();

}
}
}
