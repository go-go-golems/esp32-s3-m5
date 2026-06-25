// Native-host dashboard: C++ owns OS/app/panel/widget implementation.
const app = OS.app('dashboard')
const p = app.panel('main').frame('rounded').title(' picoOS native ').titleRight(() => OS.clock('HH:mm'))

p.text('device emulator').at(2, 2).fg('amber')
p.gauge().at(2, 4).label('batt').value(() => OS.battery).width(20).showPct()
p.text(() => OS.toast || 'press o to launch notes').at(2, 7).fg('dim')

app.key('o', () => OS.launch('notes'))
app.statusbar('o launch · q exits host')
app.mount()
