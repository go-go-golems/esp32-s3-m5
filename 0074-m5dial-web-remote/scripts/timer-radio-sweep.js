print('starting timer radio sweep')

lain.mode('radio')
lain.band('wired')
lain.station(5, 'clear', 'phantom_relay')
lain.station(22, 'distorted', 'wire_temple')
lain.station(38, 'static', 'STATIC')
lain.station(55, 'hidden', '???')
lain.station(72, 'clear', 'navi_broadcast')
lain.station(95, 'distorted', 'eiri_signal')
lain.station(112, 'clear', 'layer_07_hub')

globalThis.radioSweep && cancel(globalThis.radioSweep)

var position = 0
globalThis.radioSweep = every(180, function () {
  lain.position(position)
  position = (position + 4) % 120
})

setTimeout(function () {
  print('stopping timer radio sweep')
  cancel(globalThis.radioSweep)
  globalThis.radioSweep = null
  lain.message('radio sweep complete')
}, 6000)

'radio sweep armed'
