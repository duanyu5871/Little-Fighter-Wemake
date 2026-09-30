# [v0.1.61](https://lf.gim.ink/0.1.61)

by [**Gim**](https://gim.ink)

<!-- git-range: 8c7e38149..60849a6bb -->

### What's New

- Reconnect: dropping out mid-match no longer knocks you out of the game
  - When someone disconnects, the match pauses and a "Game Paused" notice lists the disconnected players: wait for them, or have the room owner press "Continue" to let a bot take over the disconnected fighter (if the owner is the one who dropped, ownership is handed to another player right away)
  - If you are the one who dropped, the client reconnects automatically (up to 30 attempts with delays growing from 0.3s to 3s) and shows "Reconnecting..." with the attempt count; once back, it fast-forwards through the missed ticks so you carry on seamlessly
  - The server keeps the match alive for 5 minutes waiting for reconnects (the replay frames are kept for 5 minutes too and are sent in 512-frame chunks, so even a slow reconnect can catch up); your fighter is handed back to you (including from a bot that was filling in)
  - While a bot fills in for a disconnected player, the HP bar shows that player's name as "1(bot)"; it reverts to "1" once the player is back (display only — the player's real name is never changed)
  - The replay fast-forwards (several frames per update, rendering skipped) instead of waiting in real time
  - While the returning player is fast-forwarding everyone else keeps the "waiting for reconnect" notice and resumes together once it catches up (the room owner can still press "Continue" to go on without it)
  - Not waiting? Pick "Continue Solo" (give up reconnecting and keep playing locally with bots) or "Back to Lobby" while reconnecting; if reconnect fails, the same two choices apply
- Tiered HP bars: HP / recoverable HP above 500 now extends in extra 500-point tiers with different colors (red → gold → green → steel blue → white, up to 2500)
- Death & respawn: all status bars drop to zero and hide on death, and restore smoothly after respawn

### Tweaks

- Rendering batching (CPU savings): looping background layer copies are merged into instanced draws (e.g. bg_3's 133 copies become 13 submissions); character shadows are merged into a single draw; UI node rendering gets matrix / clip dirty checks
  - Measured draw calls: 12 entities 28 → 17, 60 entities 125 → 66
- Release pipeline: installer uploads now use resumable chunked transfers with retries, so slow CI→OSS links are less likely to fail
- Netplay notices: pressing F1~F10, toggling a cheat code or using cheats like "kill all enemies" now posts a system message in the room chat saying who did it
- Netplay notices: changing your nickname or a player-slot name now posts a system message in the room chat instead of changing silently
- Player HP / MP now depend on difficulty and mode: stage mode uses a per-difficulty table (Easy 2500 / Normal 1000 / Difficult 500 / Crazy 500, MP matching HP), survival rank mode uses 2000 / 1000, and VS mode keeps the original 500 (MP at 2/5)
- Bot difficulty lowered: the AI's "defend desire" is far lower at low difficulties (bots block much less)
- Bot difficulty tiers: in stage mode, bots on a human player's team (COM slots, rescued hostages, defectors) always play at Difficult, while every other bot (VS mode included) follows the difficulty setting
- Bot pickups: on Easy they ignore every weapon, on Normal they ignore drinks

### Fixes

- "Continue" is always available and is owner-only: once a player disconnects, leaves or is kicked, the room owner can hand that fighter to a bot right away (no longer tied to the 5-minute reconnect window), and the action is broadcast to everyone in the room so all remaining players resume together instead of only the one who pressed it
- Homing balls no longer fly off-screen: the chase target point was captured before the ball reached its spawn position, so a ball with no target steered towards the world origin (Z=0 sits outside the stage range) and ended up pinned to the bottom edge of the screen; it now hovers in place
- The "team outline" toggle now applies instantly: an idle fighter (your own character in particular) used to keep its outline until its render state changed for some other reason
- Fixed recovery periods: the timers for HP / MP / fall value / defend value / toughness recovery fired twice in a row at the period boundary, so actual recovery ran at ~1.8× the nominal rate and differed between 30 and 60 UPS; they now fire exactly once per period (the MP-healing book, drinks and other per-N-frame effects are fixed along with it)
- Sharper small text: names, the `xN` reserve counter and key hints are now drawn at 4× resolution (matching the 4× render buffer), so they no longer look blurry when zoomed in or scaled
- MP recovery above 500 HP: the recovery amount is now computed from HP capped at 500, so very high-HP fighters no longer recover MP faster than intended
- Knockdown check: an airborne victim whose fall value is exactly the critical value is now knocked down as well
