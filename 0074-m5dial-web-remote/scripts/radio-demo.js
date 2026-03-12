console.log('radio-demo start')

lain.mode('radio')
lain.band('WIRED')
lain.station(35, 'clear', 'navi_broadcast')
lain.station(42, 'distorted', 'tetsuo_echo')
lain.position(35)
lain.reveal('HELLO FROM SCRIPT')

'radio-demo queued'
