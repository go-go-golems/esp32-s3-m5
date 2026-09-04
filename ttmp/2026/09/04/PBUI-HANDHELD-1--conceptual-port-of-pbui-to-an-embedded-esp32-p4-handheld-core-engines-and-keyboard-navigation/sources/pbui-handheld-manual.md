# PBUI/HB — Owner's Manual

*Firmware 0.3 · for the 320×320 arrow-key handheld · this manual matches the
browser prototype exactly, so every exercise can be checked on screen.*

---

## 0 · Before you begin

Two things to know, and the rest of this manual is just consequences of them.

**First: everything on the screen is an object.** Not a picture of an object —
the object itself. A file name is the file. A change is the change. A tile is
the tile. Each object has a *type*, and its type determines everything about
it: what color bar it wears, what pressing ⏎ does to it, which commands will
accept it, and what the bottom line of the screen says while you're on it.

**Second: the bottom line never lies.** Row 31 — the dark bar at the foot of
the screen — always tells you where you are (the mustard word on the left:
`READY`, `ACCEPT`, `CMD`…) and what your keys currently do. If you ever feel
lost, stop and read the bottom line. It is the entire user interface's
contract with you: a patient person can learn this whole device by only ever
pressing what row 31 suggests.

There is no pointer. You will not miss it.

> **Browser prototype note.** Click the device once so it has keyboard focus.
> All keys are captured; nothing needs the mouse after that first click.

---

## 1 · First light: what you're looking at

Turn the device on. It wakes on the **FILES** tile, showing a small finished
agent run called *grace-period* — a scripted coding session (eight steps of
reading, planning, editing, and testing) that ships with the device so you
have something real to explore. The screen is arranged in fixed rows:

```
┌─────────────────────────────────────────────────────┐
│ PBUI  GRACE-PERIOD      §8/8 ▮▮▮▮▮▮▮▮ ev29/29  ctx… │ ① status bar
│ ⌂triage ▸ FILES                            card 2/4 │ ② title row
│                                                     │
│ ▌ƒ src/auth/token.ts     +4  −2   signature-change  │ ③ the work
│ ▌  ◇ verifyToken   signature-change  +2 −1      §6  │    surface
│ ▌  ◇ verifyToken   add-guard         +2 −1      §7  │
│ ▌ƒ src/auth/session.ts   +8  −3   extract-function  │
│   …                                                 │
│                                                     │ ④ tray strip
│ READY <file> token.ts · ⏎ open y yank · m menu · …  │ ⑤ the doc line
└─────────────────────────────────────────────────────┘
```

**① The status bar** (dark): the run's name, how far through it the *cursor*
is (`§8/8` — all eight steps executed; `ev29/29` — the fine-grained event
position), and the context-window gauge (`ctx`), which shifts mint → mustard →
rose as the simulated agent's window fills.

**② The title row**: which workspace you're in (sage), which tile is showing,
and its position in the deck (`card 2/4`). When you drill into an object, a
breadcrumb grows here.

**③ The work surface.** Every row that begins with a small colored bar on the
left edge is an object. The bar's color is its type — you'll learn the palette
without trying: **mint** files, **rose** changes, **mustard** tasks and
window segments, **lavender** memories and steps, **blue** tool calls. One row
is always filled cream with a dotted outline: that is the **caret**, your
finger on the screen.

**④ The tray strip** (usually blank — you'll fill it in Tutorial 5).

**⑤ The doc line.** Read it now. It names the caret's object by type and
label, then lists your live options. It will change with every move you make.

---

## 2 · Tutorial: moving around *(3 minutes)*

The caret walks the objects, not the pixels.

1. Press **↓** a few times. The cream highlight steps from object to object —
   file, then its changes, then the next file. Plain text (headers, hints) is
   skipped: you can only ever stand *on something*.
2. Watch the doc line as you go. On a file it reads `<file> token.ts · ⏎
   open…`; on a change, `<hunk> verifyToken · sig… · ⏎ open R revert…`. The
   verbs on offer change because the *type* changed.
3. Press **→**. You've flipped to the next tile in the deck — **EDITS**. Press
   **→** again for **TASKS**, and **←** to walk back. A workspace is a deck of
   full-screen cards; you flip through it, you don't window-manage it.
4. Press **3**. Number keys jump straight to a card. Press **⇥** (Tab): you've
   switched to the second workspace, *context*, holding the WINDOW and MEMORY
   tiles. **⇥** again brings you home.
5. On any crowded tile, press **f**. Every object sprouts a small dark letter.
   Type the letter of something far away — the caret teleports there. This is
   the long-jump; the arrows are the walk.
6. Two more movers for later: **;** then a type letter (`;h` = hop to the next
   change, `;t` next task, `;f` `;m` `;c` `;s` likewise) hops by *kind*, and
   **/** lets you type part of a label and ⏎ to jump to it.

✔ *You should now be able to reach any object on any tile in under two
seconds. That's the prerequisite for everything else.*

---

## 3 · Tutorial: acting on things *(5 minutes)*

Every object answers three questions: *what does ⏎ do*, *what's on the menu*,
and *is there a one-key verb?*

1. Go to **FILES**, caret onto `session.ts`, press **⏎**. You've *opened* it —
   an inspector card slides in: line count, churn, its changes (which are live
   objects — note their rose bars), and the head of the file. The title row
   grew a breadcrumb.
2. Inside the inspector, caret onto the `validateSession` change and **⏎**
   again. Inspectors nest: now you're reading the change itself — its meaning
   (`extract-function · body lifted into a new named function`), where it
   happened (`§3`, a lavender step object — also live), and the diff, green
   rows added, red rows removed.
3. Press **⌫** twice to pop back out. **⌫** always retraces your drill-in,
   like a browser's back button. **Esc** does the same, one level of
   commitment at a time.
4. Hold **i** on any object. A boxed *peek* appears at the foot of the screen
   — the same inspector, but as a glance. Release **i**; you never moved. Use
   peek when ⏎ would be a commitment.
5. Press **m** on a change. This is its **menu**: every verb it answers to,
   each with a letter accelerator. Try `r` — *revert this change*. Watch the
   screen: the FILES row now says `reverted`, churn counts drop everywhere,
   because the whole world re-derives from what's true.
6. Reverting is not destructive; it's an annotation. Press **R** on the same
   change — the one-key verb — and it toggles right back to restored. The
   one-key verbs are: **R** revert/restore a change, **P** pin/unpin a memory
   or window segment, **E** evict a window segment. The doc line always shows
   the ones that apply.
7. On **TASKS**, ⏎ is different: it cycles a task todo → doing → done. Default
   verbs are per-type: files and changes *open*, tasks *cycle*, steps *rewind
   the run* (next tutorial), tiles *switch*.

✔ *Noun first, then verb: stand on the thing, then ⏎ / m / R·P·E. The other
half of the grammar — verb first — is Tutorial 4's command line.*

---

## 4 · Tutorial: the time machine *(3 minutes)*

The run you're looking at isn't a report; it's a *timeline*, and every tile is
a view of the timeline at the cursor.

1. Look at the status bar: `ev29/29`. Press **<** several times. Each press
   rewinds the cursor a whole step — watch `§8/8` fall, watch changes vanish
   from FILES *in reverse order*, watch the ctx gauge drain.
2. Press **space**. The run replays forward, one event at a time: reads land
   as window segments, edits appear with their colors, a test fails red in §6
   and passes green in §7, the window compacts at §8. **space** again pauses.
3. **,** and **.** scrub by single event when you want frame-by-frame; **<**
   and **>** jump by step.
4. Steps are objects too. Anywhere you see a lavender `§n`, you can travel to
   it: caret onto a step reference and **⏎** — the cursor jumps there. This is
   the trick to remember: *"rewind to where this happened"* is just ⏎ on any
   step object, and they're scattered everywhere — in inspectors, in memory
   rows, in the edits list.
5. Your own annotations survive time travel: revert a change, scrub to `ev0`,
   scrub back — still reverted. Pins survive §8's compaction. You are editing
   an interpretation of the run, and the run replays through it.

✔ *If a tile ever looks emptier than you expect, check `ev` in the status bar
first — you may simply be standing early in time.*

---

## 5 · Tutorial: the command line *(8 minutes — the heart of the device)*

Press **:**. The doc line becomes a prompt: `CMD » ▁`. This is the REPL, and
it has one deep idea: **the completion is read off your screen.** The device
scans the visible objects and offers only the verbs that something in front of
you can receive.

1. On **FILES**, press **:** and look at the offered verbs: `open·◇ revert·◇
   restore·◇ switch·▣ close·▣ …` — `revert` is offered because changes are
   visible. Press **Esc**, flip to **WINDOW** (⇥ then arrows, or `o` and
   dive), press **:** again — now `pin·⌸ evict·⌸` appear and `revert` is
   gone. Nothing to revert here, so it isn't offered. (Typing a verb's full
   name always works regardless — the scan curates the *menu*, not the
   *language*.)
2. Back on FILES. Type `rev` and press **␣ (space)**. Two things happen at
   once: the verb completes to `revert` (unique prefix), and *every change on
   screen lights up* — cream fill, pulsing red outline, a red digit chip on
   each. The prompt now reads:

   ```
   ACCEPT » revert <hunk> ▁ [⏎ verifyToken · it] 5 cand · digits pick lit
   ```

   You are in the argument slot, and you have four ways to fill it:

   - **Press a digit** — takes that lit object. The fastest path.
   - **Just press ⏎** — takes the *default*, shown in brackets. The default is
     the caret if it fits (`· it`), otherwise the last object of that type you
     touched. Standing on the thing you meant? The whole command was three
     keys: `: rev ␣ ⏎`... actually four. Still quick.
   - **Type a few letters** — `guard` — and watch the lit set *narrow with
     you*: only matching objects stay lit. **⇥** cycles the candidates shown
     in the prompt; ⏎ takes the current one. This reaches objects that aren't
     even on screen — the candidates come from the whole world.
   - **Esc** — changed your mind, nothing happens.
3. Do it for real: `: rev ␣` then the digit on `verifyToken ·
   signature-change`. Toast: `✓ reverted`. Now press **r** with the caret on a
   *different* change — **r repeats the last command on the caret**. Rapid
   triage is caret, r, caret, r.
4. Ambiguity is handled honestly: type `re` then ␣ — the prompt refuses and
   lists `revert · restore`. Add a letter and try again.
5. Everything you do in the REPL is echoed into the **LISTENER** (card 1) —
   and the objects in that transcript are *live*. Scroll up to yesterday's
   `revert ◇ verifyToken`, caret onto the chip, press **m**. The transcript is
   your history made of things, not text.

✔ *The grammar is verb ␣ argument, and the screen itself is the argument
picker. When a command needs a thing, the things volunteer.*

---

## 6 · Tutorial: the tray and pronouns *(5 minutes)*

The tray is how you carry objects between tiles — the pocket where a pointer
user would have dragged.

1. On **MEMORY** (⇥ to the *context* workspace), caret onto `session.ts owns
   all TTL logic` and press **y**. The tray strip opens above the doc line:
   your memory sits there as a chip with its lavender bar, addressed **$1**.
   Yank a couple more things from other tiles — each gets the next number.
   **t** hides and shows the strip; hiding doesn't empty it.
2. Tray entries are *references*, not copies. They stay live: act on the
   original and the chip reflects it. (Which also means a chip can dangle — an
   evicted segment's chip will read `(evicted)`.)
3. Spend an entry with a **pronoun**: from any tile at all, `: pin ␣` then
   type `$1` and ⏎. The slot resolves `$1` against the tray — the prompt shows
   `[⏎ session.ts owns… · $1]` so you see exactly what you're about to pin —
   and if the object is visible somewhere on the current tile, *it alone stays
   lit*. Type-checking still applies: `pin` wants a memory or segment, so a
   `$n` holding a file simply produces zero candidates rather than a wrong
   guess. The other pronoun is `it`, which nominates the caret by name.
4. Taking things off: **x** on the caret drops it from the tray (the mirror of
   **y** — and the doc line tells you which applies: it shows `x drop` only
   when the caret's object is aboard). From the REPL, `: drop ␣` is special:
   its lit targets are *only tray members*, its candidates are the tray in
   $-order, and its default is the most recent entry. `: clear ⏎` empties the
   shelf. Note that numbers renumber when something leaves — `$2` becomes `$1`.
5. Dropping removes the chip, never the object. `drop $1` on a change does not
   revert it; it just puts it out of your pocket.

✔ *y aboard · x off · t peek at the shelf · $n to spend from anywhere.*

---

## 7 · Tutorial: tiles, decks, and the overview *(5 minutes)*

Tiles obey the same law as everything else: **a tile is an object.** It has a
label (`[EDITS] · triage`), verbs, an inspector, and it can ride the tray.

1. Press **o**. This is the **overview**: your whole session as a map — sage
   workspace names, each tile as a ▣ chip with a one-line vital sign (`+16
   −6`, `2 doing`, `43%`). Arrows choose, **⏎ dives**, **o** or **Esc**
   returns. **m** on any chip here opens the tile's own verb menu: switch,
   close, open a new tile, inspect, yank.
2. Make a tile: `: newtile ␣`. The slot accepts an *app* — type `ed`, ⏎. A
   second **EDITS** tile appears in the deck right after your current card,
   and you're on it. Two EDITS tiles are genuinely independent: park one on an
   interesting change, keep triaging in the other.
3. Close a tile: `: close ␣ ⏎`. Read the prompt before you confirm: `[⏎
   [EDITS] · triage · this tile]`. When nothing card-like is under your caret,
   **the current tile offers itself as the default** — so `close ␣ ⏎` means
   "close the one I'm looking at." To close a *different* one, type part of
   its name in the slot, or do it from the overview. The device refuses to
   close a workspace's last tile.
4. `: switch ␣` + a few letters of a tile's name teleports anywhere — the
   fastest cross-workspace jump. And any ▣ chip anywhere (overview, listener
   transcript, tray) answers ⏎ with *switch to me*.
5. Notice the REPL consequence: because the current tile is always implicitly
   "visible," the tile verbs are *always offered* in completion, on every
   screen. The tile exposing its own actions is not a feature bolted on — it
   falls out of the one rule.

✔ *Deck = flip with ←→. Session = map with o. Tiles = objects like anything
else.*

---

## 8 · How to think in PBUI *(the ideas behind the keys)*

**Types do the work.** When the device knows a slot wants a `<hunk>`, it can
light every hunk, complete over every hunk's name, and refuse everything else.
That's why commands are declared with typed signatures, and why you never get
the wrong kind of thing into a command: the mistake is unrepresentable.

**Two grammars, one vocabulary.** Noun-first: stand on it, then ⏎ / m / R.
Verb-first: `:`, name the verb, let the nouns volunteer. Both routes end at
the same verbs; use whichever your hands are already doing.

**Defaults are memory.** Every slot has a best guess — the caret ("it"), the
current tile, the last thing of that type you touched, the newest tray entry —
and the prompt always *shows* the guess in brackets before you commit. Bare ⏎
is never a gamble; it's the printed default.

**The screen is a question, the world is the answer.** Tiles don't own their
data; they re-derive it from the timeline at the cursor, filtered through your
annotations (reverts, pins, statuses). That's why scrubbing time updates
everything at once, and why your marks survive replay.

**Say less, mean more.** Unique prefixes complete (`rev ␣`), pronouns
substitute (`$1`, `it`), `r` repeats, digits pick. The design goal is that the
common case is always one to three keys.

---

## 9 · Reference

### 9.1 The object types

| Bar · Glyph | Type | ⏎ default | One-key verbs | Notes |
|---|---|---|---|---|
| mint ƒ | `file` | open inspector | y/x | inspector lists its changes, live |
| rose ◇ | `hunk` | open (diff) | **R** revert⇄restore, y/x | wears its sem-class color |
| mustard ☐◐☑ | `task` | cycle status | y/x | glyph shows todo/doing/done |
| lavender μ | `mem` | open | **P** pin⇄unpin, y/x | pinning materializes a window segment |
| mustard ⌸ | `ctxseg` | open | **P** pin, **E** evict, y/x | pins survive compaction |
| lavender § | `step` | rewind run here | y/x | appears as `§n` cross-references everywhere |
| blue ⚙ | `toolcall` | open | y/x | green ok / red ERR |
| ink ▣ | `card` (tile) | switch to it | y/x | always offers its verbs to the REPL |
| sage ▦ | `app` | — | — | only ever appears as `newtile`'s argument |

### 9.2 Keys by mode

**NAV (READY)** — the home mode
| Key | Action |
|---|---|
| ↑ ↓ (j k) | caret to prev/next object |
| ← → ([ ]) | prev/next card in the deck |
| 1…9 | jump to card n |
| ⇥ | next workspace |
| ⏎ | the object's default verb |
| ⌫ | pop one drill-in |
| f | hint labels → type one to jump |
| ; + f h t m c s o | hop to next file/hunk/task/mem/seg/step/tool |
| / | label search in this tile · ⏎ jumps |
| : | open the command line |
| m | verb menu for the caret |
| R P E | revert⇄restore · pin⇄unpin · evict |
| y x t | tray: yank · drop · toggle strip |
| r | repeat last command on the caret |
| i (hold) | peek inspector, no navigation |
| o | overview |
| space | play / pause the run |
| , . | scrub one event back / forward |
| < > | scrub one step back / forward |
| ? | help card |
| Esc | up one level: mode → peek → drill-in |

**CMD** (after `:`) — type a verb · **⇥** cycle offered verbs · **␣** complete
verb and enter the argument slot · **⏎** on a bare verb = same as ␣ · Esc.

**ACCEPT** (the argument slot) — **digits** pick lit targets *(only while the
slot is empty)* · **letters** filter, and the lit set narrows live · **$n**
tray entry · **it** the caret · **⇥** cycle candidates · **⏎** take shown
`[⏎ …]` · Esc.

**MENU** — ↑↓ + ⏎, or press the item's letter · Esc.
**OVERVIEW** — ↑↓ choose · ⏎ dive · m tile verbs · o / Esc.
**HELP** — any key returns.

### 9.3 Command table

| Command | Argument | Effect |
|---|---|---|
| `open` | anything | push its inspector (⌫ pops) |
| `revert` / `restore` | ◇ hunk | annotate a change out of / back into the world |
| `pin` / `unpin` | ⌸ seg or μ mem | keep in the window ⇄ release (mem pin creates its segment) |
| `evict` | ⌸ seg | remove from the window now |
| `forget` | μ mem | strike a memory (its segment leaves too) |
| `done` / `start` | ☐ task | set status |
| `goto` | § step | move the run cursor there |
| `switch` | ▣ tile | jump to that tile, any workspace |
| `close` | ▣ tile | close it — *default: the current tile* |
| `newtile` | ▦ app | open a tile after this one and go there |
| `yank` / `drop` | anything | tray on / off (`drop` lights only tray members) |
| `clear` | — | empty the tray |
| `help` | — | the key card |

Availability: a command is *offered* in completion when something visible can
receive it; tile verbs always; `drop`/`clear` only while the tray holds
something. Any command *typed in full* runs regardless.

### 9.4 Reading the chrome

- **Status bar**: run name · `§done/total` + step gauge · `ev` cursor · ctx
  gauge (mint <70% < mustard <90% < rose).
- **Title row**: `⌂workspace ▸ TILE ▸ breadcrumbs…` · `card n/m`.
- **Doc line**: `MODE` in mustard, then context. In READY it names the caret's
  object and live verbs; in ACCEPT it shows the prompt, the bracketed default,
  and the candidate count.

---

## 10 · If something seems wrong

**"Nothing lights up when I press ␣."** The slot's type has no visible
instances on this tile. The candidates in the prompt still cover the whole
world — type a few letters, or Esc and go where the objects are.

**"It says ambiguous."** Your verb prefix matches several commands (`re` →
revert, restore). One more letter.

**"A verb I want isn't in the completion."** The scan didn't find a visible
receiver (or the tray is empty, for tray verbs). Type the full name — it
always works — or ask why the object isn't on screen.

**"$1 didn't do anything."** Check the type: the prompt shows `0 cand` when
the tray entry can't fill that slot (e.g. `pin ␣ $1` where $1 is a file).
Also remember $-numbers *renumber* when entries are dropped.

**"The tile looks empty / says (not yet)."** You're early in time. Look at
`ev` in the status bar; press `>` or space.

**"A tray chip says (evicted)."** The reference outlived its object. `x` it
off, or restore the object.

**"It won't close my tile."** A workspace keeps at least one card. Switch
something else in first, or close from a fuller workspace.

**Truly lost?** `Esc` until READY, then `?`. And the bottom line never lies.

---

*PBUI/HB is a prototype of a presentation-based UI in the tradition of CLIM
and Genera's Dynamic Windows: the screen as typed, living objects rather than
pictures. No pointers were harmed, or included.*
