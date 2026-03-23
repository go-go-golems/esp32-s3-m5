// numbers-station.js
// Simulates a shortwave numbers station. The dial locks onto a
// frequency and broadcasts groups of numbers/words at intervals,
// like UVB-76 or the Lincolnshire Poacher.

print('numbers-station: tuning in')

globalThis.numStation && cancel(globalThis.numStation)
globalThis.numDrift && cancel(globalThis.numDrift)

lain.mode('radio')
lain.band('shortwave')

// Our station sits at position 47
var stationPos = 47

// Seed the band with interference
lain.station(12, 'static', 'STATIC')
lain.station(30, 'static', 'STATIC')
lain.station(47, 'distorted', 'UVB-76')
lain.station(68, 'static', 'STATIC')
lain.station(85, 'static', 'STATIC')
lain.station(102, 'distorted', 'S-4921')

// Slowly drift toward the station
var tunePos = 0
var tuneHandle = every(100, function () {
  lain.position(tunePos)
  if (tunePos < stationPos) {
    tunePos = tunePos + 1
  } else {
    cancel(tuneHandle)
    tuneHandle = null
    lain.station(stationPos, 'clear', 'UVB-76')
    lain.message('signal locked')
    startBroadcast()
  }
})

// Number groups in the style of real number stations
var groups = [
  '7 3 0 8 5',
  '2 9 1 4 6',
  '0 0 7 7 3',
  '5 1 8 2 9',
  '4 6 3 0 1',
  '9 9 2 5 8',
  '1 7 4 6 0',
  '3 8 5 2 7',
]

var words = [
  'ZHUZH',
  'MDZHB',
  'KONTROL',
  'NOMER',
  'SIGNAL',
  'KANAL',
  'SEKTOR',
  'PRIЁМ',
]

var broadcastIdx = 0

function startBroadcast() {
  // Preamble
  lain.reveal('ATTENTION ATTENTION')

  setTimeout(function () {
    lain.reveal('MESSAGE BEGINS')
  }, 2000)

  // Start number groups after preamble
  setTimeout(function () {
    globalThis.numStation = every(2200, function () {
      var gIdx = broadcastIdx % groups.length
      var wIdx = broadcastIdx % words.length

      if (broadcastIdx % 4 === 3) {
        // Every 4th message is a word
        lain.reveal(words[wIdx])
        lain.message('word: ' + words[wIdx])
      } else {
        lain.reveal(groups[gIdx])
        lain.message('group: ' + groups[gIdx])
      }

      // Small position wobble to simulate tuning instability
      var wobble = (Date.now() % 5) - 2
      lain.position(stationPos + wobble)

      broadcastIdx++
    })
  }, 4000)

  // After 12 groups, the station drifts away
  setTimeout(function () {
    if (globalThis.numStation) {
      cancel(globalThis.numStation)
      globalThis.numStation = null
    }

    lain.reveal('MESSAGE ENDS')
    lain.message('end of transmission')

    // Drift off frequency
    var driftPos = stationPos
    globalThis.numDrift = every(200, function () {
      driftPos = driftPos + 2
      lain.position(driftPos)
      lain.station(stationPos, 'distorted', 'UVB-76')
      if (driftPos > 110) {
        cancel(globalThis.numDrift)
        globalThis.numDrift = null
        lain.station(stationPos, 'static', 'STATIC')
        lain.message('signal lost')
        print('numbers-station: complete')
      }
    })
  }, 30000)
}

'tuning to shortwave'
