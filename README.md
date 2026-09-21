<div align="center">

# StartPos Linker

Tie an **online level** to one of your **created levels** and bounce between them with a single tap.

![GD 2.2](https://img.shields.io/badge/GD-2.2081-blue?style=for-the-badge)
![Geode](https://img.shields.io/badge/Geode-5.10.1-green?style=for-the-badge)
![C++](https://img.shields.io/badge/C%2B%2B-17-orange?style=for-the-badge)

</div>

---

## What it does

Have you ever wanted to speedrun a copy of someone's level, or test an online level you can't publish? **StartPos Linker** lets you pair any online level with one of your own created levels — then travel between the two instantly.

The link button turns into a **switch** once a pair is made, so going back and forth is always one click away.

---

## How to use

**1. Link**

- On an **online level** page, tap the green circle with the flag (🏁).
- On your **created level** page, tap the green circle with the hammer (🔨).

Order doesn't matter — press one, then the other. The moment both sides have spoken, they're linked.

**2. Switch**

Once linked, that same spot becomes a **blue arrow button**. Tap it:

- On the online level → jumps you into your created copy.
- On your created level → opens the online original.

**3. Unlink**

Tap the small **red ✕** that appears under the switch to break the pair.

---

## Why no level IDs?

In newer Geometry Dash versions, created levels don't report a stable numeric ID — so instead of bugging you with an ID box, **StartPos Linker tracks created levels by their name** (the same way the game itself does). No typing, no reading long numbers, just two taps.

> Note: renaming the created level will drop its link, but the online half is unaffected.

---

## Feature highlights

- 🟢 **Name-based linking** — no numeric IDs, no manual input.
- 🔄 **One-tap switching** between an online level and its local copy.
- 🧹 **One-tap unlinking** with a dedicated ✕ button.
- 📏 **Adaptive placement** — buttons dock to the existing button columns, so it stays tidy even when other mods add their own buttons.
- 💾 **Persisted pairs** — links survive restarts; they're saved to your mod settings.

---

## Install

From the [Geode Mod Index](https://geode-sdk.org) or a release `.geode` file dropped into your mods folder.

## Building

Requires [Geode SDK](https://github.com/geode-sdk/geode) 5.x and GD 2.2081.

```bash
geode build
```

The packaged `sleepen.level-linker.geode` is written straight into your mods folder.

---

<div align="center">

Made for the GD modding community. If you use it, enjoy the flow.  
**sleepen**

</div>