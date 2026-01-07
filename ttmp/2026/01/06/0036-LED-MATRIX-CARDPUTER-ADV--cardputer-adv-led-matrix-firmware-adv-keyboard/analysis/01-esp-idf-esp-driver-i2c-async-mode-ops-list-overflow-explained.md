---
Title: 'ESP-IDF esp_driver_i2c async mode: ops list overflow explained'
Ticket: 0036-LED-MATRIX-CARDPUTER-ADV
Status: active
Topics:
    - esp-idf
    - esp32s3
    - cardputer
    - console
    - usb-serial-jtag
    - keyboard
    - sensors
    - serial
    - debugging
    - flashing
DocType: analysis
Intent: long-term
Owners: []
RelatedFiles:
    - Path: ../../../../../../../../../../esp/esp-idf-5.4.1/components/esp_driver_i2c/i2c_master.c
      Note: Driver internals for async queueing and 'ops list is full' guardrail
    - Path: 0036-cardputer-adv-led-matrix-console/main/matrix_console.c
      Note: Keyboard bus config (trans_queue_depth) and the observed error
    - Path: 0036-cardputer-adv-led-matrix-console/main/tca8418.c
      Note: TCA8418 init performs many small I2C transactions
ExternalSources: []
Summary: ""
LastUpdated: 2026-01-06T23:10:17.508681737-05:00
WhatFor: ""
WhenToUse: ""
---


# ESP-IDF `esp_driver_i2c` async mode: why “ops list is full” happened

This document explains (in excruciating detail) why the Cardputer-ADV keyboard bring-up failed with:

```
E i2c.master: s_i2c_asynchronous_transaction(...): ops list is full, please increase your trans_queue_depth
E i2c.master: i2c_master_multi_buffer_transmit(...): I2C transaction failed
W kbd: keyboard init failed: ESP_ERR_INVALID_STATE
```

…and why the fix (“set `trans_queue_depth = 0`”) resolves it.

The goal is that after reading this, you can answer questions like:

- What is “async mode” in ESP-IDF’s I²C driver?
- What is an “ops list” and *why is it finite*?
- Why did the ops list fill up during a simple register init sequence?
- What are the correct ways to use async mode if we ever need it?
- Why does this have nothing to do with “the I²C peripheral is broken” (and everything to do with the *driver configuration and pacing*)?

## 0) The problem in one sentence

We accidentally enabled the driver’s asynchronous transaction queue by setting `trans_queue_depth > 0`, and then we executed a burst of many small I²C register writes (TCA8418 init) faster than the hardware could complete them; the driver only had room for `trans_queue_depth` pending operation lists, so it hit the guardrail and returned `ESP_ERR_INVALID_STATE`.

## 1) Context: what we were trying to do (Cardputer-ADV keyboard bring-up)

The Cardputer-ADV keyboard is a **TCA8418** keypad controller:

- It sits on I²C (SDA=`GPIO8`, SCL=`GPIO9` in the ADV docs / demo firmware)
- It asserts an interrupt line (`INT=GPIO11`) when events are available
- It stores key events in an internal FIFO (`KEY_EVENT_A` / `KEY_LCK_EC`)

Our firmware does a “classic embedded init” sequence:

1. Configure the I²C master bus
2. Configure the TCA8418 registers (many single-register writes)
3. Clear any pending FIFO/events
4. Enable interrupts
5. Start a task that drains events when INT fires

The important part for this document is step (2): **it is many discrete register writes**.

### 1.1 What “many discrete writes” looks like

The TCA8418 init we modeled after the ADV demo performs writes like:

- `GPIO_DIR_1/2/3`
- `GPI_EM_1/2/3`
- `GPIO_INT_LVL_1/2/3`
- `GPIO_INT_EN_1/2/3`
- keypad matrix size: `KP_GPIO_1/2/3`
- enable interrupts: set bits in `CFG`

That is already ~15+ I²C write transactions, and that’s before any “flush” or readbacks.

If each write is issued as its own I²C transaction, you *must* either:

- perform them synchronously (each one waits until it’s done), or
- have enough queue depth and/or pacing to handle the burst.

## 2) ESP-IDF I²C driver architecture (mental model)

ESP-IDF 5.x has a newer I²C driver API in `esp_driver_i2c`:

- A **bus handle** represents one I²C controller instance (I2C0 / I2C1)
- A **device handle** represents a specific target address + speed on that bus
- Transactions are expressed internally as a small “program” of operations (“ops”) that drive the I²C hardware state machine

If you only learn one diagram from this document, learn this:

```
Your code
  |
  |  i2c_master_transmit() / i2c_master_transmit_receive()
  v
esp_driver_i2c (software)
  |
  |  builds an operation-list (ops) describing START/WRITE/READ/STOP
  v
I2C hardware peripheral (state machine + FIFOs + interrupts)
  |
  v
SCL/SDA pins on the wire
```

### 2.1 The “ops list” is not your data; it is the driver’s instruction list

When the driver says “ops list is full”, it does **not** mean:

- the I²C FIFO is full
- the bus is stuck
- the device isn’t ACKing
- your wires are wrong

It means:

- the driver has an internal pool (array) that stores queued **operation sequences**
- that pool has a finite size equal to `trans_queue_depth`
- your code asked the driver to queue more transactions than that pool can hold before earlier transactions completed

So “ops list” is a **software resource**.

## 3) Two modes: synchronous vs asynchronous

ESP-IDF’s bus config controls whether the driver uses a synchronous (blocking) path or an asynchronous (queued) path.

### 3.1 The switch: `trans_queue_depth`

In ESP-IDF 5.4’s `i2c_new_master_bus()`:

- If `trans_queue_depth == 0`:
  - `async_trans` stays false
  - Every transmit call uses the synchronous transaction function
  - Your function call blocks until the transaction completes or times out
- If `trans_queue_depth > 0`:
  - `async_trans` is set true
  - The driver prints a warning that async is “experimental”
  - Every transmit call queues work into a transaction queue and returns immediately (subject to internal pool limits)

This is why you saw:

```
W i2c.master: Please note i2c asynchronous is only used for specific scenario currently. It's experimental ...
```

You didn’t call “async APIs”; you just set `trans_queue_depth` non-zero.

### 3.2 Synchronous mode: what it feels like (timeline)

In synchronous mode, your code and the bus stay in lock-step.

Diagram (not to scale):

```
CPU time                                  I2C bus time
--------                                  ----------
call write(reg,val)  ------------------>  START + addr + reg + val + STOP
wait (blocked)       <------------------  completes
call write(reg,val)  ------------------>  next transaction...
```

Key property:

- You can issue 100 register writes and you will not “overflow” anything,
  because you never have more than ~1 transaction pending at once.

### 3.3 Async mode: what it feels like (timeline)

In async mode, your code can enqueue transactions much faster than the bus can execute them.

```
CPU time (very fast)                       I2C bus time (slower)
-------------------                       ----------------------
enqueue write #1  --------------------->  TX #1 running...
enqueue write #2  --------------------->  (queued)
enqueue write #3  --------------------->  (queued)
...
enqueue write #8  --------------------->  (queued)
enqueue write #9  ----X ERROR: ops list full (depth=8)
```

The bus didn’t “fail” electrically; we just ran out of queue slots.

## 4) What exactly is stored in the “ops list”?

Internally, each I²C transaction is stored as:

1. A small array of `i2c_operation_t` entries (“ops”)
2. A transaction descriptor that points at that ops array and includes the device address
3. A set of FreeRTOS queues holding transaction descriptors in various states

Think of it like a tiny instruction program:

```
ops for a simple register write:

  OP0: RESTART/START
  OP1: WRITE bytes [reg, value]   (driver also handles device address)
  OP2: STOP
```

For a write-then-read (typical “read register”):

```
ops for reg read:

  OP0: START
  OP1: WRITE bytes [reg]
  OP2: START (repeated start)
  OP3: READ N bytes (ACK...NACK)
  OP4: STOP
```

This operations list is what gets copied into the driver’s per-bus working buffer and then fed to the hardware in chunks.

## 5) The internal queues (diagram)

When async mode is enabled, the driver sets up several queues.

At a high level:

```
             (free descriptors)                (queued to run)            (done, waiting recycle)
           +-------------------+             +---------------+           +----------------------+
           | READY queue       |             | PROGRESS      |           | COMPLETE queue       |
           | tx descriptor pool|             | queue         |           |                      |
           +-------------------+             +---------------+           +----------------------+
                    |                                     ^                         |
                    | allocate descriptor                  | ISR dequeues + runs     | user calls wait_all_done()
                    v                                     |                         v
             +-------------------+                         +-------------------------+
             | inflight counter  |
             +-------------------+
```

And separately, there is the **ops storage pool**:

```
ops storage (size = trans_queue_depth):

  i2c_async_ops[0]  i2c_async_ops[1]  ... i2c_async_ops[N-1]
     ^                 ^                       ^
     |                 |                       |
  transaction A     transaction B           transaction N
```

The error you hit:

> “ops list is full, please increase your trans_queue_depth”

means:

- the driver tried to allocate a slot in `i2c_async_ops[...]`
- but `ops_cur_size == queue_size` (queue_size == trans_queue_depth)
- so there was no free slot to store the next operation list

## 6) Why did it fill during TCA8418 init specifically?

Because TCA8418 init is “many writes”, and each write was a separate transaction.

Let’s be concrete.

### 6.1 Rough transaction count during init

Our init does (conceptually):

1. 3 writes: `GPIO_DIR_1/2/3`
2. 3 writes: `GPI_EM_1/2/3`
3. 3 writes: `GPIO_INT_LVL_1/2/3`
4. 3 writes: `GPIO_INT_EN_1/2/3`
5. 2–3 writes: `KP_GPIO_1/2/3` (depends on matrix cols)
6. a few reads/writes for flush + enable interrupts

This exceeds 8 transactions quickly.

So if `trans_queue_depth == 8`, the 9th call will fail *unless some earlier transaction finished and freed an ops slot*.

### 6.2 Why didn’t earlier transactions free slots quickly enough?

Because bus time is slower than CPU time.

At 400 kHz, a minimal register write is on the order of:

- address byte + ACK: 9 clock cycles
- register byte + ACK: 9 clock cycles
- value byte + ACK: 9 clock cycles
- plus START/STOP overhead and internal driver latency

Even ignoring overhead, that’s 27 clock cycles:

```
27 cycles / 400,000 cycles/s ≈ 67.5 µs
```

Now compare:

- Enqueueing a transaction in software is a few microseconds (or less)
- Completing a transaction on the bus is ~70–150+ µs depending on overhead

So your code can enqueue 10–20 transactions before the first one finishes.

That’s *exactly* the scenario where a small queue depth overflows.

## 7) The specific driver behavior that matters (pseudocode)

Here is the relevant shape of the ESP-IDF implementation (simplified pseudocode).

### 7.1 “Enable async mode if trans_queue_depth > 0”

```c
// in i2c_new_master_bus(...)
if (bus_config->trans_queue_depth > 0) {
  async_trans = true;
  queue_size = bus_config->trans_queue_depth;
  allocate trans_queues[READY, PROGRESS, COMPLETE] with depth=queue_size
  allocate i2c_async_ops[queue_size][MAX_OPS]
}
```

### 7.2 “Transmit” in async mode allocates an ops slot (or fails)

```c
// in i2c_master_transmit() -> i2c_master_multi_buffer_transmit() -> s_i2c_asynchronous_transaction()

if (sent_all && num_trans_inqueue == 0) {
  // fast-path: bus idle, start immediately using the fixed working ops buffer
  copy ops into i2c_master->i2c_ops
  start transaction
} else {
  // bus already busy; queue it
  take(bus_lock)
  if (ops_cur_size == queue_size) {
    give(bus_lock)
    return ESP_ERR_INVALID_STATE   // "ops list is full"
  }
  ops_cur_size++
  slot = ops_prepare_idx
  copy ops into i2c_async_ops[slot]
  ops_prepare_idx = (ops_prepare_idx + 1) % queue_size
  give(bus_lock)

  // allocate a transaction descriptor and push to PROGRESS queue
  desc = pop(READY or COMPLETE)
  desc.ops = i2c_async_ops[slot]
  push(PROGRESS, desc)
  num_trans_inqueue++
  num_trans_inflight++
}
```

Key point:

- The driver **must store the ops list somewhere** until the hardware is ready to run it.
- That storage is `i2c_async_ops[queue_size]`.
- You can’t enqueue more than `queue_size` outstanding transactions.

### 7.3 “Completion frees an ops slot”

When a transaction finishes, the ISR path decrements `ops_cur_size`, freeing an ops slot.

Conceptually:

```c
ISR_on_transaction_complete() {
  if (queue_trans) {
    ops_cur_size--;
    push(COMPLETE, desc);
  }

  if (num_trans_inqueue > 0) {
    // pop next desc and start it
  }
}
```

So:

- Async mode works fine as long as “enqueue rate” ≤ “completion rate + queue capacity”
- We violated that by doing a tight burst of register writes

## 8) Why the driver returns `ESP_ERR_INVALID_STATE`

The specific error code comes from a guardrail:

- `ops_pool == (ops_cur_size != queue_size)`
- if false, return `ESP_ERR_INVALID_STATE`

This is not “I²C bus invalid state”; it is “driver’s internal ops pool is exhausted”.

## 9) Fix strategies (in increasing complexity)

There are several legitimate fixes; we chose the simplest and most correct for our use case.

### 9.1 Fix A (chosen): keep the bus synchronous

Set:

- `trans_queue_depth = 0`

Effect:

- Every I²C transaction blocks until complete
- There is no ops pool to overflow
- Error reporting is more straightforward

This is what we committed in `0036-cardputer-adv-led-matrix-console/main/matrix_console.c`.

### 9.2 Fix B: increase `trans_queue_depth` substantially

If you truly want async mode:

- set `trans_queue_depth` to a number larger than your worst-case burst length
  - for TCA init, 32 would be “safe enough”

Downside:

- consumes more RAM for operation buffers and queue structures
- you must still reason about pacing (bursts can still exceed the new depth)
- the driver warns async mode is “experimental” and has different error semantics

### 9.3 Fix C: pace enqueues (wait between groups)

You can keep async mode but avoid overflow by not enqueueing faster than completion:

- enqueue a few transactions
- wait until they complete (via callbacks or `i2c_master_bus_wait_all_done`)
- enqueue the next batch

Conceptually:

```c
for each register write:
  i2c_master_transmit(...)
  i2c_master_bus_wait_all_done(bus, timeout_ms)  // blocks until queue drains
```

At that point, you’ve reinvented synchronous mode, but with extra overhead.

### 9.4 Fix D: reduce transaction count (combine writes)

Sometimes you can write multiple consecutive registers in a single I²C transaction if:

- the device supports auto-increment register addressing, and
- you enable that mode, and
- the register map is contiguous for what you’re doing

For TCA8418, there is an “auto-increment” bit (`CFG_AI`), but using it safely requires careful ordering and knowledge of which registers may be sensitive.

This is higher risk for bring-up and not necessary for our current goals.

## 10) How this maps back to the observed logs

You saw:

1. The async-mode warning at startup
2. Immediately after, the “ops list is full” error
3. Then `kbd init failed: ESP_ERR_INVALID_STATE`

That is exactly what happens when:

- we set `trans_queue_depth = 8`
- we issue ~15 register writes quickly
- the first few calls fill the ops pool
- the 9th call hits the guardrail before earlier ones complete

Changing to synchronous mode (`trans_queue_depth = 0`) means:

- each write completes before the next begins
- ops pool is not used
- the init sequence cannot “outpace” the hardware

## 11) Practical debugging checklist (if keyboard still fails after the fix)

Once async mode is disabled, failures you see are more likely to be “real I²C problems”:

- Wrong pins / wrong device address
- Missing pull-ups or too-weak pull-ups
- Bus speed too high for wiring
- Device not powered / held in reset
- INT wiring incorrect (though INT affects events, not basic register writes)

Typical symptoms and what they mean:

- `ESP_ERR_TIMEOUT` on transmit:
  - SDA/SCL stuck, device not responding, pull-ups absent, wrong address, bus held low
- `NACK` event logged (if enabled):
  - wrong address or device not present
- Intermittent failures:
  - power integrity, wiring noise, shared bus contention

## 12) Appendix: why this is especially common in “register init” code

Many embedded peripherals require a “burst of register writes” at boot.

Synchronous mode is usually a perfect fit because:

- boot-time init is not performance-critical
- determinism matters more than throughput
- error handling is simpler (you know which exact write failed)

Async mode shines when:

- you have multiple devices on one bus
- you want to pipeline transactions while doing other work
- you have a structured pacing mechanism (callbacks/tasks) and a good reason to overlap work

For our keyboard bring-up, we want:

- simplicity
- correct error propagation
- predictable sequencing

So synchronous mode is the right default.

