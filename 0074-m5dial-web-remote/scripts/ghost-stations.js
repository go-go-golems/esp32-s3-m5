// ghost-stations.js
// Stations flicker in and out of existence. The dial scans on its own,
// pausing when it finds something, then losing the signal.

print('ghost-stations: initializing')

globalThis.ghost && cancel(globalThis.ghost)
globalThis.ghostScan && cancel(globalThis.ghostScan)
globalThis.ghostReveal && cancel(globalThis.ghostReveal)

lain.mode('radio')
lain.band('ETHER')

var ghosts = [
  { pos: 7,   type: 'hidden',    name: '???' },
  { pos: 19,  type: 'distorted', name: 'dead_air' },
  { pos: 33,  type: 'clear',     name: 'someone_there' },
  { pos: 48,  type: 'hidden',    name: '...' },
  { pos: 61,  type: 'static',    name: 'STATIC' },
  { pos: 77,  type: 'distorted', name: 'old_broadcast' },
  { pos: 88,  type: 'hidden',    name: 'do_not_listen' },
  { pos: 103, type: 'clear',     name: 'last_signal' },
  { pos: 115, type: 'distorted', name: 'echo_echo' },
]

var reveals = [
  'CAN YOU HEAR ME',
  'THE WIRED REMEMBERS',
  'CLOSE YOUR EYES',
  'NO ONE IS HERE',
  'KEEP LISTENING',
  'SIGNAL LOST',
  'YOU WERE WARNED',
  'LAYER 07',
]

// Randomly show/hide stations to make them ghostly
var flickerIdx = 0
globalThis.ghost = every(700, function () {
  var g = ghosts[flickerIdx % ghosts.length]
  // Coin flip: station appears or vanishes
  if ((Date.now() % 3) === 0) {
    lain.station(g.pos, 'clear', '')
  } else {
    lain.station(g.pos, g.type, g.name)
  }
  flickerIdx++
})

// Slow scanner that pauses near ghost stations
var scanPos = 0
var scanSpeed = 2
var dwellTicks = 0
globalThis.ghostScan = every(120, function () {
  if (dwellTicks > 0) {
    dwellTicks--
    return
  }

  lain.position(scanPos)

  // Check proximity to any ghost station
  var i = 0
  while (i < ghosts.length) {
    var dist = scanPos - ghosts[i].pos
    if (dist < 0) dist = -dist
    if (dist <= 2) {
      // Dwell near the station
      dwellTicks = 8
      break
    }
    i++
  }

  scanPos = (scanPos + scanSpeed) % 120
})

// Occasional cryptic reveals
var revealIdx = 0
globalThis.ghostReveal = every(3500, function () {
  lain.reveal(reveals[revealIdx % reveals.length])
  revealIdx++
})

// Stop after 30 seconds
setTimeout(function () {
  cancel(globalThis.ghost)
  cancel(globalThis.ghostScan)
  cancel(globalThis.ghostReveal)
  globalThis.ghost = null
  globalThis.ghostScan = null
  globalThis.ghostReveal = null
  lain.reveal('TRANSMISSION ENDED')
  lain.message('ghost stations: silence')
  print('ghost-stations: complete')
}, 30000)

'ghost stations active'
