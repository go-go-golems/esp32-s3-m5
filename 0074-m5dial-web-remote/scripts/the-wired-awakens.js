// the-wired-awakens.js
// A narrative boot sequence. The dial wakes up, discovers it is
// connected to the Wired, and stations appear one by one as if
// the network is coming online.

print('the-wired-awakens: booting')

globalThis.wiredBoot && cancel(globalThis.wiredBoot)

lain.mode('radio')
lain.band('offline')
lain.position(0)

// Narrative timeline — each entry fires at the given ms offset
var timeline = [
  {  ms: 0,     fn: function () { lain.message('initializing...') } },
  {  ms: 1200,  fn: function () { lain.message('scanning frequencies...') } },
  {  ms: 2000,  fn: function () { lain.band('searching') } },
  {  ms: 2500,  fn: function () { lain.position(12) } },
  {  ms: 2800,  fn: function () { lain.position(24) } },
  {  ms: 3100,  fn: function () { lain.position(36) } },
  {  ms: 3400,  fn: function () { lain.position(48) } },
  {  ms: 3700,  fn: function () { lain.position(60) } },
  {  ms: 4000,  fn: function () { lain.message('carrier detected') } },
  {  ms: 4500,  fn: function () { lain.band('WIRED') } },
  {  ms: 5000,  fn: function () {
    lain.station(15, 'distorted', 'handshake')
    lain.position(15)
    lain.message('node 01 online')
  }},
  {  ms: 6200,  fn: function () {
    lain.station(15, 'clear', 'protocol_7')
    lain.message('handshake complete')
  }},
  {  ms: 7500,  fn: function () {
    lain.station(40, 'static', 'STATIC')
    lain.position(40)
    lain.message('noise on channel 40')
  }},
  {  ms: 8800,  fn: function () {
    lain.station(40, 'distorted', 'ghost_node')
    lain.message('resolving...')
  }},
  {  ms: 10000, fn: function () {
    lain.station(40, 'clear', 'cyberia_cafe')
    lain.message('node 02 stable')
  }},
  {  ms: 11500, fn: function () {
    lain.station(65, 'hidden', '???')
    lain.position(65)
    lain.message('encrypted signal')
  }},
  {  ms: 13000, fn: function () {
    lain.reveal('PRESENT DAY')
  }},
  {  ms: 14500, fn: function () {
    lain.reveal('PRESENT TIME')
  }},
  {  ms: 16000, fn: function () {
    lain.station(65, 'clear', 'lain')
    lain.message('identity confirmed')
  }},
  {  ms: 17500, fn: function () {
    lain.station(90, 'distorted', 'eiri_masami')
    lain.position(90)
    lain.message('unauthorized node detected')
  }},
  {  ms: 19000, fn: function () {
    lain.reveal('YOU DONT NEED TO BE AFRAID')
  }},
  {  ms: 20500, fn: function () {
    lain.station(110, 'clear', 'knights')
    lain.position(110)
    lain.message('network mapped')
  }},
  {  ms: 22000, fn: function () {
    lain.position(65)
    lain.reveal('NO MATTER WHERE YOU GO')
  }},
  {  ms: 24000, fn: function () {
    lain.reveal('EVERYONE IS CONNECTED')
  }},
  {  ms: 26000, fn: function () {
    lain.band('LAYER 07')
    lain.message('the wired is awake')
  }},
]

// Fire each timeline event with setTimeout
var i = 0
while (i < timeline.length) {
  // Use an IIFE to capture the current index
  (function (entry) {
    setTimeout(function () { entry.fn() }, entry.ms)
  })(timeline[i])
  i++
}

// Gentle idle sweep after the narrative finishes
setTimeout(function () {
  var pos = 65
  var dir = 1
  globalThis.wiredBoot = every(300, function () {
    lain.position(pos)
    pos = pos + (dir * 3)
    if (pos >= 115) dir = -1
    if (pos <= 10) dir = 1
  })
}, 27000)

// Full stop at 45s
setTimeout(function () {
  if (globalThis.wiredBoot) {
    cancel(globalThis.wiredBoot)
    globalThis.wiredBoot = null
  }
  lain.message('connection closed')
  print('the-wired-awakens: complete')
}, 45000)

'the wired is booting'
