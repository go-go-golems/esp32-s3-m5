# `s3paper` — a fluent JS API for e-ink e-readers

Design goals: **fluent chains** for ergonomics, **small composable primitives** underneath, **opinionated defaults** tuned for e-ink (partial refresh, ghosting control, chunky touch targets), and **lambdas everywhere** a decision might need to be overridden.

Core primitives: `paper` (the device/display), `region` (a rectangular update zone), widgets (`text`, `row`, `col`, `divider`, `progress`, `list`), and `page` (a routable full-screen unit). Everything composes; the fluent layer is sugar on top.

---

## 1. Hello world

```js
import { paper, text } from "s3paper";

paper()
  .page(text("Hello, ink.").size("xl").center())
  .render();
```

Opinionated defaults do the work: full refresh on first render, grayscale-4 mode, portrait orientation. You write *what*, the runtime decides *how* to push it to the panel.

---

## 2. Composing primitives — a status bar

```js
import { row, text, icon, spacer } from "s3paper";

const statusBar = row(
  icon("battery").bind(() => sys.battery()),   // lambda = live data source
  spacer(),
  text(() => clock.format("HH:mm")),           // text accepts string OR lambda
)
  .height(24)
  .invert();  // black bar, white text — one call
```

Widgets accepting **either a value or a lambda** is the core customization pattern. A lambda marks the widget as *dynamic*, which automatically scopes it to a partial-refresh region — no manual dirty-rect bookkeeping.

---

## 3. A reading view — pagination as a first-class concern

```js
import { page, book, gesture } from "s3paper";

page("reader")
  .content(
    book(currentEpub)
      .font("Bookerly", 11)
      .margins({ x: 42, y: 36 })
      .hyphenate(lang => lang !== "ja")        // lambda: per-language decision
  )
  .on(gesture.tapRight, b => b.nextPage())
  .on(gesture.tapLeft,  b => b.prevPage())
  .on(gesture.swipeDown, () => nav.push("toc"))
  .render();
```

`book()` is a primitive that owns text layout + pagination. Page turns use **partial refresh by default**, but the runtime tracks ghosting and injects a full refresh every N turns — you never think about it (unless you want to; see below).

---

## 4. Opting into refresh control — the escape hatch lambda

```js
paper()
  .refreshPolicy(ctx =>
    ctx.turnsSincefull > 8 || ctx.ghostingScore > 0.3
      ? "full"
      : "partial"
  )
  .page(reader)
  .render();
```

The opinionated default (`every 6 turns → full`) lives behind a policy lambda. `ctx` exposes what the runtime already measures — turn count, estimated ghosting, ambient temperature (e-ink cares!) — so custom policies stay declarative.

---

## 5. Reactive regions — updating a clock without redrawing the page

```js
import { region, text } from "s3paper";

const clockZone = region(text(() => clock.format("HH:mm")).size("sm"))
  .at({ top: 4, right: 8 })
  .every("1m")           // scheduler built in
  .quiet();              // suppress refresh while user is reading (input-idle aware)

page("reader").overlay(clockZone);
```

`region` is the primitive under all partial updates. `.quiet()` is an opinion baked in: e-ink flicker mid-sentence is hostile, so timed regions defer until input has been idle.

---

## 6. Builder pattern for a full screen — the library

```js
import { page, list, col, text, divider } from "s3paper";

page("library")
  .header(text("Library").size("lg"), { pinned: true })
  .content(
    list(shelf.books)
      .item(b =>
        col(
          text(b.title).weight("bold").ellipsis(),
          text(b.author).size("sm").gray(2),
          progress(b.percentRead).height(3),
        ).padding(12)
      )
      .separator(divider().dotted())
      .sort((a, b) => b.lastOpened - a.lastOpened)   // lambdas again
      .onSelect(b => nav.push("reader", { book: b }))
      .paginate("fit")   // fit items to screen height, page — never scroll-blur on e-ink
  )
  .footer(statusBar)
  .render();
```

Two opinions worth calling out:

- **`.paginate("fit")` instead of scrolling.** Smooth scrolling is an anti-pattern on e-ink; the API doesn't offer it. Lists page.
- **`.item(lambda)`** — the list doesn't know what a book row looks like; you compose one from primitives per item. Composability over configuration objects.

---

## 7. Everything together — a tiny complete app

```js
import { app, paper, gesture } from "s3paper";

app()
  .device(paper().rotation(0).grayscale(4))
  .pages(libraryPage, readerPage, tocPage)
  .home("library")
  .on(gesture.longPress, () => nav.push("settings"))
  .sleep({
    after: "10m",
    screensaver: ctx => coverArt(ctx.currentBook),  // lambda picks the sleep image
  })
  .boot();
```

`app()` is the outermost builder: device config, routing, global gestures, power management. Note `sleep.screensaver` — even the sleep screen is just a lambda returning a widget, so the whole surface area stays uniform: **primitives + composition + lambdas, all the way down.**

---

## The shape of the design, in one table

| Layer | What it is | Customization point |
|---|---|---|
| `app` | boot, routing, power | global gestures, sleep lambda |
| `page` | full-screen unit | header/content/footer/overlay slots |
| `region` | partial-refresh zone | schedule + quiet policy |
| widgets | `text`, `row`, `col`, `list`, `book`… | value-or-lambda props, `.item()` composers |
| `paper` | the panel itself | `refreshPolicy` lambda |

The demo pitch: *the API encodes e-ink expertise as defaults (pagination, ghosting management, quiet updates), but every opinion is a lambda you can replace.*
