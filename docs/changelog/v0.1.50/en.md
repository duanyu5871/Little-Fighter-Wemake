# [v0.1.50](https://lf.gim.ink/0.1.50)

by [**Gim**](https://gim.ink)

<!-- git-range: d3d6cb08..bf78f8f3 -->

### Tweaks

- Survival single-player ranks (non-Bilibili) are now keyed by client + fighter, so one client's different fighters each get their own row
- Survival rank now reports the same phase number as the game itself (no more +1)
- The frame rate option in settings now has a note: the rendered frame rate is limited by your device and display refresh rate, so the selected value may not be reached
- Airborne fighters are easier to knock down
- Longer hit stop and hurt shaking (6 → 8); ball weapons have a much longer hit stop (2 → 8)
- Magic flute buffs are stronger: damage 1 → 2, duration 2 → 3
- Book weapons are much more fragile: bundle durability 800 → 400 with drop threshold 200 → 180; each book durability 750 → 200 with drop threshold 48 → 20; the bundle's hit damage 60 → 50; how books are held was also tweaked
- Julian's D>A: hover overshoot ("reverse only after passing the target by this much") is now per-axis and acceleration is higher; the ball and AI look for targets more often (every 2 frames, timed per entity)
- Scoreboard names are simplified: human players show their player name, other fighters show the fighter name
- Build tool: identical input now produces identical output; PNGs are optimized, so data packs are smaller

### Fixes

- A thrown weapon that hits someone now enters its dropped state, so its attack no longer works after it lands
- Whether you can recover from a magic flute hit is now checked via the buff itself (no longer via collision records, so it does not break after the victim flies away)
- Fixed caught fighters not leaving the caught state when a catching move transforms the catcher
- Fixed "Little Fighter 2" being misspelled as "Litter Fighter 2" in the launcher texts

### Thanks

- Thanks to "Sauce" and "布利.白" for their feedback, support and encouragement
- Thanks to everyone who has tried LFW
