# [v0.1.60](https://lf.gim.ink/0.1.60)

by [**Gim**](https://gim.ink)

<!-- git-range: 26db79883..4df133cd3 -->

### What's New

- Reconnect: dropping out mid-match no longer knocks you out of the game
  - When someone disconnects, the match pauses and a "Game Paused" notice lists the disconnected players: wait for them, or hit "Continue" to let a bot take over the disconnected fighter and keep playing
  - If you are the one who dropped, the client reconnects automatically (up to 30 attempts with delays growing from 0.3s to 3s) and shows "Reconnecting..." with the attempt count; once back, it fast-forwards through the missed ticks so you carry on seamlessly
  - The server keeps the match alive for 5 minutes waiting for reconnects; your fighter is handed back to you (including from a bot that was filling in)
  - Not waiting? Hit "Back to Lobby" while reconnecting; if reconnect fails, pick "Continue Solo" or "Back to Lobby"
- Tiered HP bars: HP / recoverable HP above 500 now extends in extra 500-point tiers with different colors (red → gold → green → steel blue → white, up to 2500)
- Death & respawn: all status bars drop to zero and hide on death, and restore smoothly after respawn

### Tweaks

- Rendering batching (CPU savings): looping background layer copies are merged into instanced draws (e.g. bg_3's 133 copies become 13 submissions); character shadows are merged into a single draw; UI node rendering gets matrix / clip dirty checks
  - Measured draw calls: 12 entities 28 → 17, 60 entities 125 → 66
- Release pipeline: installer uploads now use resumable chunked transfers with retries, so slow CI→OSS links are less likely to fail
