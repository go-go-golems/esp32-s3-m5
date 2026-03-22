# ═══════════════════════════════════════════════════════════════
#  GNOSIS LAYOUT ENGINE — Algorithmic Specification
#  For 1-bit EPD framebuffer on memory-constrained devices
#
#  Conventions:
#    - SMALL-CAPS     = procedure names
#    - italic (w,h,x) = variables
#    - ▷              = inline comment
#    - ←              = assignment
#    - Arrays are 1-indexed
#    - All units in pixels, origin at top-left
# ═══════════════════════════════════════════════════════════════


# ───────────────────────────────────────────
#  §1  DATA STRUCTURES
# ───────────────────────────────────────────

struct Rect
    x, y        : uint16        ▷ top-left corner
    w, h        : uint16        ▷ width and height

struct Node
    type         : enum { VBOX, HBOX, FIXED, LABEL, BAR, LIST,
                          GRID, CIRCLE, CROSS, SEP, BTN, COND }
    rect         : Rect         ▷ computed bounding box
    children     : Node[]       ▷ max 16 children per node
    n_children   : uint8
    props        : uint16[4]    ▷ type-specific properties packed
    bind         : uint8        ▷ index into binding table, or NULL
    dirty        : bool         ▷ needs redraw
    waveform     : enum { FULL, FAST, PART }

struct Screen
    bar          : Node         ▷ top bar
    body         : Node         ▷ main content tree
    nav          : Node         ▷ bottom nav bar


# ───────────────────────────────────────────
#  §2  MAIN ENTRY POINT
# ───────────────────────────────────────────

procedure LAYOUT-SCREEN(screen, W, H)
───────────────────────────────────────────
    Input:  screen — a Screen struct
            W, H   — display width and height in pixels
    Output: all Node.rect fields populated with final positions

    ▷ Phase 1: allocate vertical bands for bar, body, nav
    bar_h  ← screen.bar.props[0]         ▷ fixed bar height from DSL
    nav_h  ← screen.nav.props[0]
    body_h ← H − bar_h − nav_h

    LAYOUT-NODE(screen.bar,  0, 0,             W, bar_h)
    LAYOUT-NODE(screen.body, 0, bar_h,         W, body_h)
    LAYOUT-NODE(screen.nav,  0, bar_h + body_h, W, nav_h)


# ───────────────────────────────────────────
#  §3  RECURSIVE LAYOUT DISPATCH
# ───────────────────────────────────────────

procedure LAYOUT-NODE(node, x, y, w, h)
───────────────────────────────────────────
    Input:  node — the node to lay out
            x, y — allocated top-left position
            w, h — allocated width and height
    Output: node.rect set; children recursively laid out

  1   node.rect ← Rect(x, y, w, h)
  2
  3   if node.type = VBOX
  4       LAYOUT-VBOX(node, x, y, w, h)
  5   else if node.type = HBOX
  6       LAYOUT-HBOX(node, x, y, w, h)
  7   else if node.type = FIXED
  8       LAYOUT-FIXED(node, x, y, w, h)
  9   else
 10       LAYOUT-LEAF(node, x, y, w, h)     ▷ terminal widget


# ───────────────────────────────────────────
#  §4  VERTICAL BOX LAYOUT
# ───────────────────────────────────────────

procedure LAYOUT-VBOX(node, x, y, w, h)
───────────────────────────────────────────
    ▷ Two-pass algorithm:
    ▷   Pass 1 — sum fixed-height children, count flexible ones
    ▷   Pass 2 — distribute remaining space, assign positions

    ▷ Pass 1: measure
  1   fixed_total ← 0
  2   flex_count  ← 0
  3
  4   for i ← 1 to node.n_children
  5       child ← node.children[i]
  6       if HAS-EXPLICIT-HEIGHT(child)
  7           fixed_total ← fixed_total + EXPLICIT-HEIGHT(child)
  8       else
  9           flex_count ← flex_count + 1

    ▷ Pass 2: assign
 10   remaining ← h − fixed_total
 11   flex_h ← 0
 12   if flex_count > 0
 13       flex_h ← ⌊remaining / flex_count⌋
 14
 15   cursor_y ← y
 16
 17   for i ← 1 to node.n_children
 18       child ← node.children[i]
 19       if HAS-EXPLICIT-HEIGHT(child)
 20           child_h ← EXPLICIT-HEIGHT(child)
 21       else
 22           child_h ← flex_h
 23
 24       LAYOUT-NODE(child, x, cursor_y, w, child_h)
 25       cursor_y ← cursor_y + child_h


# ───────────────────────────────────────────
#  §5  HORIZONTAL BOX LAYOUT
# ───────────────────────────────────────────

procedure LAYOUT-HBOX(node, x, y, w, h)
───────────────────────────────────────────
    ▷ Identical to VBOX but on the horizontal axis.
    ▷ Special case: if node has "split" property, use
    ▷ two-pane fixed+remainder model (common in GNOSIS DSL).

  1   if HAS-SPLIT(node)
  2       split_w ← node.props[1]            ▷ left pane fixed width
  3       right_w ← w − split_w − 1          ▷ 1px for divider
  4
  5       ▷ left pane
  6       LAYOUT-NODE(node.children[1], x, y, split_w, h)
  7       ▷ right pane
  8       LAYOUT-NODE(node.children[2], x + split_w + 1, y, right_w, h)
  9       return

    ▷ General case: same two-pass as VBOX, on width axis
 10   fixed_total ← 0
 11   flex_count  ← 0
 12
 13   for i ← 1 to node.n_children
 14       child ← node.children[i]
 15       if HAS-EXPLICIT-WIDTH(child)
 16           fixed_total ← fixed_total + EXPLICIT-WIDTH(child)
 17       else if IS-SPACER(child)
 18           flex_count ← flex_count + 1
 19       else
 20           fixed_total ← fixed_total + TEXT-WIDTH(child)  ▷ measure label

 21   remaining ← w − fixed_total
 22   flex_w ← 0
 23   if flex_count > 0
 24       flex_w ← ⌊remaining / flex_count⌋

 25   cursor_x ← x

 26   for i ← 1 to node.n_children
 27       child ← node.children[i]
 28       if IS-SPACER(child)
 29           child_w ← flex_w
 30       else if HAS-EXPLICIT-WIDTH(child)
 31           child_w ← EXPLICIT-WIDTH(child)
 32       else
 33           child_w ← TEXT-WIDTH(child)
 34
 35       LAYOUT-NODE(child, cursor_x, y, child_w, h)
 36       cursor_x ← cursor_x + child_w


# ───────────────────────────────────────────
#  §6  FIXED (ABSOLUTE) POSITIONING
# ───────────────────────────────────────────

procedure LAYOUT-FIXED(node, x, y, w, h)
───────────────────────────────────────────
    ▷ Children carry explicit (x, y) offsets relative to parent.
    ▷ No flow. Each child is placed independently.

  1   for i ← 1 to node.n_children
  2       child ← node.children[i]
  3       cx ← x + child.props[0]            ▷ local x offset
  4       cy ← y + child.props[1]            ▷ local y offset
  5       cw ← child.props[2]                ▷ explicit width, or w if 0
  6       ch ← child.props[3]                ▷ explicit height, or remaining

  7       if cw = 0 then cw ← w − child.props[0]
  8       if ch = 0 then ch ← h − child.props[1]

  9       LAYOUT-NODE(child, cx, cy, cw, ch)


# ───────────────────────────────────────────
#  §7  LEAF WIDGET MEASUREMENT
# ───────────────────────────────────────────

procedure LAYOUT-LEAF(node, x, y, w, h)
───────────────────────────────────────────
    ▷ Terminal nodes. No children to recurse into.
    ▷ Rect already set by caller. Compute content metrics
    ▷ only where needed for later rendering.

  1   node.rect ← Rect(x, y, w, h)

  2   case node.type of

  3       LABEL:
  4           ▷ content width = len(text) × GLYPH_W × size_multiplier
  5           ▷ if width exceeds w and truncate=true, store
  6           ▷   visible_chars ← ⌊w / (GLYPH_W × size)⌋
  7           size ← node.props[0]             ▷ 1, 2, or 4
  8           if size = 0 then size ← 1
  9           node.props[2] ← ⌊w / (GLYPH_W × size)⌋   ▷ max visible chars

 10       BAR:
 11           ▷ bar fill width computed at render time from bound value
 12           ▷ nothing to precompute

 13       LIST:
 14           ▷ visible_rows ← min(node.props[1], ⌊h / node.props[2]⌋)
 15           ▷   props[1] = max items, props[2] = row_h
 16           node.props[3] ← min(node.props[1], ⌊h / node.props[2]⌋)

 17       GRID:
 18           ▷ cell dimensions from column count and cell size
 19           ▷ rows_visible ← ⌊h / cell_h⌋
 20           cols ← node.props[0]
 21           cell_w ← node.props[1]
 22           cell_h ← node.props[2]
 23           if cell_w = 0 then cell_w ← ⌊w / cols⌋
 24           node.props[1] ← cell_w           ▷ write back computed
 25           node.props[3] ← ⌊h / cell_h⌋    ▷ visible rows

 26       CIRCLE, CROSS, SEP, BTN:
 27           pass                              ▷ fully defined by rect


# ───────────────────────────────────────────
#  §8  TEXT MEASUREMENT HELPER
# ───────────────────────────────────────────

function TEXT-WIDTH(node) → uint16
───────────────────────────────────────────
    ▷ For bitmap font, width is trivially character count × glyph width.
    ▷ Bound values use worst-case width (field width from format spec).

  1   GLYPH_W ← 8                            ▷ pixels per glyph
  2   size ← max(1, node.props[0])            ▷ size multiplier

  3   if IS-BOUND(node)
  4       n_chars ← BIND-FIELD-WIDTH(node.bind)  ▷ e.g. "%03d" → 3
  5   else
  6       n_chars ← STRLEN(node.text)

  7   return n_chars × GLYPH_W × size


# ───────────────────────────────────────────
#  §9  DIRTY REGION TRACKING
# ───────────────────────────────────────────

procedure MARK-DIRTY(node)
───────────────────────────────────────────
    ▷ Called when a bound value changes.
    ▷ Marks the node and propagates up to compute
    ▷ the minimal enclosing refresh rectangle.

  1   node.dirty ← true
      ▷ parent marking not needed — we collect dirty rects
      ▷ bottom-up at render time


procedure COLLECT-DIRTY-RECTS(node, rects[], count)
───────────────────────────────────────────
    ▷ Walk tree. Gather bounding boxes of dirty leaves.
    ▷ Merge overlapping rects to reduce EPD refresh calls.

  1   if node.dirty
  2       if node.n_children = 0               ▷ leaf
  3           rects[count] ← node.rect
  4           count ← count + 1
  5           node.dirty ← false
  6       else
  7           for i ← 1 to node.n_children
  8               COLLECT-DIRTY-RECTS(node.children[i], rects, count)
  9           node.dirty ← false
 10   else
 11       for i ← 1 to node.n_children        ▷ dirty child under clean parent
 12           if node.children[i].dirty
 13               COLLECT-DIRTY-RECTS(node.children[i], rects, count)


procedure MERGE-RECTS(rects[], n) → merged[], m
───────────────────────────────────────────
    ▷ Greedy merge: combine any two rects whose union area
    ▷ is less than sum of individual areas + threshold.
    ▷ Threshold prevents merging distant small regions into
    ▷ one large refresh (expensive on e-ink).

  1   THRESHOLD ← 512                         ▷ pixels², tunable

  2   repeat
  3       merged_any ← false
  4       for i ← 1 to n
  5           for j ← i + 1 to n
  6               u ← UNION-RECT(rects[i], rects[j])
  7               waste ← AREA(u) − AREA(rects[i]) − AREA(rects[j])
  8               if waste < THRESHOLD
  9                   rects[i] ← u
 10                   REMOVE(rects, j)
 11                   n ← n − 1
 12                   merged_any ← true
 13                   break                    ▷ restart inner loop
 14   until not merged_any

 15   return rects[1..n]


# ───────────────────────────────────────────
#  §10  EPD REFRESH DISPATCH
# ───────────────────────────────────────────

procedure REFRESH-DISPLAY(screen, framebuf)
───────────────────────────────────────────
    ▷ Renders dirty regions to framebuffer, then issues
    ▷ EPD partial refresh commands with appropriate waveform.

  1   rects[32] ← {}                          ▷ static array, max 32 regions
  2   count ← 0

  3   COLLECT-DIRTY-RECTS(screen.bar,  rects, count)
  4   COLLECT-DIRTY-RECTS(screen.body, rects, count)
  5   COLLECT-DIRTY-RECTS(screen.nav,  rects, count)

  6   if count = 0 then return                 ▷ nothing changed

  7   merged ← MERGE-RECTS(rects, count)

  8   for i ← 1 to |merged|
  9       r ← merged[i]
 10       wf ← WORST-WAVEFORM-IN-RECT(screen, r)  ▷ see below
 11       RENDER-RECT(screen, framebuf, r)     ▷ rasterize nodes into buf
 12       EPD-PARTIAL-REFRESH(framebuf, r, wf) ▷ hardware driver call


function WORST-WAVEFORM-IN-RECT(screen, r) → waveform
───────────────────────────────────────────
    ▷ If any dirty node within rect r requires FULL, use FULL.
    ▷ Otherwise if any requires PART, use PART.
    ▷ Otherwise use FAST.
    ▷ This is conservative — prevents ghosting artifacts.

  1   worst ← FAST

  2   for each dirty node n overlapping r
  3       if n.waveform = FULL
  4           return FULL
  5       if n.waveform = PART
  6           worst ← PART

  7   return worst


# ───────────────────────────────────────────
#  §11  RENDERING PRIMITIVES
# ───────────────────────────────────────────

procedure RENDER-RECT(screen, framebuf, clip)
───────────────────────────────────────────
    ▷ Walk the screen tree. For each leaf node whose rect
    ▷ intersects clip, invoke the appropriate draw routine.

  1   RENDER-SUBTREE(screen.bar,  framebuf, clip)
  2   RENDER-SUBTREE(screen.body, framebuf, clip)
  3   RENDER-SUBTREE(screen.nav,  framebuf, clip)


procedure RENDER-SUBTREE(node, framebuf, clip)
───────────────────────────────────────────
  1   if not RECTS-INTERSECT(node.rect, clip)
  2       return                               ▷ early cull

  3   if node.n_children = 0                   ▷ leaf
  4       DRAW-WIDGET(node, framebuf, clip)
  5   else
  6       for i ← 1 to node.n_children
  7           RENDER-SUBTREE(node.children[i], framebuf, clip)


procedure DRAW-WIDGET(node, buf, clip)
───────────────────────────────────────────
  1   r ← INTERSECT-RECT(node.rect, clip)     ▷ clipped bounds

  2   case node.type of

  3       LABEL:
  4           text ← RESOLVE-BIND(node)
  5           size ← max(1, node.props[0])
  6           max_ch ← node.props[2]
  7           for i ← 0 to min(STRLEN(text), max_ch) − 1
  8               gx ← node.rect.x + i × GLYPH_W × size
  9               gy ← node.rect.y
 10               BLIT-GLYPH(buf, gx, gy, text[i], size, r)

 11       BAR:
 12           val ← RESOLVE-BIND-INT(node)
 13           max_val ← node.props[0]
 14           fill_w ← ⌊node.rect.w × val / max_val⌋
 15           FILL-RECT(buf, node.rect.x, node.rect.y,
                         fill_w, node.rect.h, FG, r)

 16       CIRCLE:
 17           cx ← node.props[0]
 18           cy ← node.props[1]
 19           radius ← node.props[2]
 20           DRAW-CIRCLE-BRESENHAM(buf, cx, cy, radius, r)

 21       CROSS:
 22           cx ← node.props[0]
 23           cy ← node.props[1]
 24           len ← node.props[2]
 25           HLINE(buf, cx − len, cy, 2 × len, r)
 26           VLINE(buf, cx, cy − len, 2 × len, r)

 27       SEP:
 28           HLINE(buf, node.rect.x, node.rect.y,
                    node.rect.w, r)

 29       BTN:
 30           STROKE-RECT(buf, node.rect, r)
 31           ▷ label drawn as child LABEL node

 32       LIST:
 33           ▷ iterate visible rows, draw each row's template
 34           ▷ at y offsets = row_index × row_h
 35           visible ← node.props[3]
 36           row_h   ← node.props[2]
 37           offset  ← SCROLL-OFFSET(node)
 38           for i ← 0 to visible − 1
 39               data_i ← offset + i
 40               ry ← node.rect.y + i × row_h
 41               DRAW-LIST-ROW(node, buf, data_i, ry, clip)

 42       GRID:
 43           cols   ← node.props[0]
 44           cell_w ← node.props[1]
 45           cell_h ← node.props[2]
 46           items  ← RESOLVE-BIND-ARRAY(node)
 47           for i ← 0 to |items| − 1
 48               col ← i mod cols
 49               row ← ⌊i / cols⌋
 50               gx ← node.rect.x + col × cell_w
 51               gy ← node.rect.y + row × cell_h
 52               DRAW-GRID-CELL(node, buf, items[i], gx, gy,
                                  cell_w, cell_h, clip)


# ───────────────────────────────────────────
#  §12  COMPLEXITY ANALYSIS
# ───────────────────────────────────────────

    ▷ Let N = total nodes in screen tree (typically 30–60).
    ▷ Let D = number of dirty rects (typically 1–4).
    ▷ Let P = pixels in largest dirty rect.
    ▷
    ▷ LAYOUT-SCREEN:    O(N)        — single tree walk
    ▷ COLLECT-DIRTY:    O(N)        — single tree walk
    ▷ MERGE-RECTS:      O(D²)       — D ≤ 32, effectively constant
    ▷ RENDER-RECT:      O(N + P)    — cull + pixel fill
    ▷ EPD-REFRESH:      O(D × P)    — hardware-bound, ~50–300ms each
    ▷
    ▷ Memory:
    ▷   Node struct:     16 bytes   (rect=8, props=8, pointers packed)
    ▷   Screen tree:     ~60 nodes  = 960 bytes
    ▷   Dirty rect buf:  32 × 8     = 256 bytes
    ▷   Framebuffer:     400×300/8  = 15,000 bytes
    ▷   ────────────────────────────────────────
    ▷   Total:                       ~16.5 kB
    ▷   Remaining for stack + data:  ~3.5 kB
