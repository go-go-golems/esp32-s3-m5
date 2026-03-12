print('starting timer reveal demo')

globalThis.revealHandle && cancel(globalThis.revealHandle)

var messages = [
  'HELLO',
  'LAIN',
  'WIRED',
  'SIGNAL',
]

var index = 0
lain.mode('radio')
globalThis.revealHandle = every(1000, function () {
  lain.reveal(messages[index % messages.length])
  index++
})

setTimeout(function () {
  print('stopping timer reveal demo')
  cancel(globalThis.revealHandle)
  globalThis.revealHandle = null
  lain.message('reveal demo complete')
}, 5500)

'reveal demo armed'
