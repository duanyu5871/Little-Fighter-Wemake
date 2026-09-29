# [v0.1.59](https://lf.gim.ink/0.1.59)

by [**Gim**](https://gim.ink)

<!-- git-range: af9fdeb17..642008f14 -->

### What's New

- Netplay nicknames: set one in the connection panel (up to 10 characters, remembered automatically); it shows up in the prepare screen, next to cursors, etc.
- Mouse input is fully enabled: you can click / hover the UI during netplay matches (it used to be locked online)
  - Cursors show the operator's nickname, so you can see at a glance who is pointing
  - The local cursor is now rendered independently and without easing, so it tracks the mouse much more closely (remote cursors keep their smoothing)
- Move table: ◀ ▶ buttons on both sides of the character name; click to switch characters (same as the L / R keys)
- Netplay room list reworked: pagination, an icon toolbar (create / refresh / auto-refresh / disconnect) and re-laid-out room cards
- Desktop client
  - Update checks and download progress now show in the bottom-right corner of the game page (no more dialogs), with a "restart to install" button when the download is ready
  - Ctrl+Shift+I toggles DevTools anytime (game and tool windows); the tray menu has an entry for it too
  - New tray item "Open previewer"; the previewer can go back to the game; closing the game window now exits the app completely (no leftover process)

### Tweaks

- Memory & performance: BGM is streamed instead of fully decoded into memory; texture / background / shadow rendering use less memory; character controllers are pooled and reused
- Data pack loading progress is reported as a single overall percentage
- Dev previewer: the "image" tab is now a "resource" tab with 3D model preview and file sizes; desktop window controls match the game page
- Fixed: a character could get stuck on the same frame for a long time when hit rapidly and repeatedly
- Fixed: typing into an input field also triggered in-game actions
