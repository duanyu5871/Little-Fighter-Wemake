# Thoughts on the Differences from LF2

[中文](./对于与LF2表现差异的看法.MD) | [English](./对于与LF2表现差异的看法.EN.MD)

Sadly, this project was not implemented by decompiling the original, and it updates at 60 frames per second — so "fully reproducing the original behavior" became a nearly impossible task.

And by the time the code architecture had stabilized, going back to find a way to reference the original code was already very difficult to do.

---

An imperfect analogy: the Spider-Man movies — three lead actors, three series.

The three series give the audience completely different feelings; the only things that are the same might be the character names and some character relationships.

LFW is roughly like that: the names are still those names, the moves are still those moves, but "game feel" — something built from countless tiny details — is very hard to replicate exactly through black-box work.

Early in development, I sometimes obsessed too much over "making certain combos from the original work".

I kept adjusting parameters, which made other combos stop working; new combos even appeared, like "the scythe can hit repeatedly". Fix one problem and another floats up — and in the end, only one thing was certain: with the code differing, tuning parameters to approximate the original's behavior is a road without an end.

Side effects like "the scythe can hit repeatedly" don't necessarily have to be killed as bugs. As long as they're not seriously destructive (freezes, save corruption), they can also be a seed for new gameplay. Whether to keep or change them depends on whether they're good or bad for the game — not on whether "the original had them".

Of course, the "techniques" that emerge in all kinds of modified data packs — ones the original data never had — should still be supported where support is due.

---

In the end, I still hope everyone can lower their expectations of LFW a little when it comes to replicating the original.

Of course, this is not to say that subjective feelings don't matter — quite the opposite: when you've played the original for a long time, your intuition about rhythm and feel was built over countless matches, and "this doesn't feel right" is itself a completely real experience. I fully understand it, and I'm willing to listen. I just hope you can treat it as "different", rather than judging it outright as "wrong".

Everyone is also very welcome to fork and open pull requests. Though I should be upfront: my coding style is pretty messy, so please bear with me.

And in front of AI, that seems to have become a very small problem — anyone can easily reshape it into what they want, which of course includes adding new things to it.

A project that relies only on "replication" has a ceiling — the original's content will eventually be more or less fully covered, while for a game to stay fun forever, it has to be able to hold new things: new characters, new moves, new mechanics, even gameplay the original could never produce. Staying alive means it can't go without new content. The only precondition for adding new content to LFW is not breaking the default experience.

Finally, I also don't want LFW to invite criticism of the original — it has no intention of belittling it, and its problems belong to itself alone.

---

Written by Gim / Polished by DeepSeek V4 Flash
