# [v0.1.55](https://lf.gim.ink/0.1.55)

by [**Gim**](https://gim.ink)

<!-- git-range: 61e6ae33..9bfa1ac3 -->

### What's New

- Bilibili danmu game: viewers join the fight from chat
  - Three modes: Free-for-All, 8-Team Melee and Co-op Challenge
  - Enter the room or send a message to join (Co-op claims an unclaimed Template slot first; the other modes join directly while there is room), then send a fighter nickname (e.g. 「拳王」 for Davis, 「奶妈」 for John) to switch to that fighter
  - Co-op scales enemy counts with the fighters on stage; queue cap 50, stage cap 32, and 5 minutes without any interaction removes you from the queue
  - Cheer keywords (default: 加油 / 666 / 应援) heal your fighter for 15% (3s cooldown); gifts, guards and super chats count as cheers too
  - The in-game panel rotates hints, and the "switch fighter" hint shows 3 random fighter nicknames
  - Scoreboard: spawns, kills, deaths, damage and cheers accumulate per viewer with configurable weights and persist across sessions; the built-in board page works as an OBS browser source (auto refresh every 10s)
- LFW desktop client (Bilibili interactive playable)
  - Frameless window; tray menu to toggle the co-op server, allow LAN access to the game page, copy the LAN address, open the data tool, copy the data tool command, open the data folder, show the game window and quit
  - Bundled co-op server, data tool and converters (ffmpeg and ImageMagick included) — no Node install required; drop an LF2 folder (or a conf file) onto start.exe to start converting
  - The co-op server picks the next free port when the default one is taken
  - Standalone installer with auto-update: checked at startup, downloaded in the background, applied on restart
  - Menu and hint texts are multilingual (follows the in-game language, fully customizable); config lives in danmu.json5 with a full help.md bundled
- Volume buttons (BGM / SFX) now pop up a vertical slider on hover (mouse environments)
- While playing online, a new settings button and panel: key bindings and input devices, master / BGM / SFX volume, language, team outline, render rate and stats display — all without interrupting the game
- Opoint generation expressions: spawn position (`gen_x` / `gen_y` / `gen_z`), initial velocity (`gen_dvx` / `gen_dvy` / `gen_dvz`) and spreading (`gen_spread_x` / `gen_spread_y` / `gen_spread_z`)
  - Variables `w` / `h` / `cx` / `cy`; functions `rand` / `pick` / `bag` (draws without repeats) / `flip` / `round`
  - Examples: `rand(-w/6, w/6)`, `round(rand(round(w/4), round(3*w/4)))`

### Tweaks

- Burning smoke is now driven by generation-point expressions
- Demo mode lineups and titles adjusted
- Data packs (zip) are now decompressed on demand

### Thanks

- Thanks to "Sauce" (酱油), "布利.白" and the QQ group members I can't name, for their feedback, support and encouragement
- Thanks to the many more who have tried LFW
