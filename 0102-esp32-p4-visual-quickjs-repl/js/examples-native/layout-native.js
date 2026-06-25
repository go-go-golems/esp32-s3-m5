// Native-host layout example: rows and panel regions are owned by C++.
const app = OS.app('layout')
app.layout(l => l.row(1, 'bar').row('*', 'body'))

app.panel('bar').frame('rounded').title(' native layout ').titleRight(() => OS.clock('HH:mm'))
const body = app.panel('body').frame('rounded')
body.text('layout regions').at('center', 2).bold().fg('cyan')
body.gauge().at(4, 5).label('batt').value(() => OS.battery).width(18).showPct()

app.statusbar('layout native · q exits host')
app.mount()
