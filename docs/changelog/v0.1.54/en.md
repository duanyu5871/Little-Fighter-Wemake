# [v0.1.54](https://lf.gim.ink/0.1.54)

by [**Gim**](https://gim.ink)

<!-- git-range: 6f011c5a..2190304b -->

### What's New

- New "stand-attack distance" data for fighters and weapons: a dead zone and a max distance
  - Bots now back off first when too close, then attack from a standstill
  - Bots will not stand and attack from beyond the max distance
  - Defaults are set for each weapon type and can be overridden in data
- Frames now support a `blinking` field in XML, the editor and at runtime

### Tweaks

- While backing off, bots keep a small gap on the Z axis instead of retreating on the same line
- Bots pinned in a corner now slip out of the corner first
- Bots switch to attacking when they can no longer open up distance, instead of hugging the edge doing nothing

### Fixes

- Fixed buff effects sometimes lingering, and sometimes showing up at the origin (0,0)

### Thanks

- Thanks to "Sauce" (酱油), "布利.白" and the QQ group members I can't name, for their feedback, support and encouragement
- Thanks to the many more who have tried LFW
