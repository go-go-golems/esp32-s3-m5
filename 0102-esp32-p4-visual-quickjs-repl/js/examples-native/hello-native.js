// Native-host picoOS example: API implemented by C++ QuickJS bindings.
const app = OS.app('hello')
const st = app.state({ n: 0, last: '' })
const p = app.panel('main').frame('rounded').title(' hello native ')

p.text('C++ backed picoOS').at('center', 2).bold().fg('cyan')
p.text(() => 'ticks: ' + st.n).at('center', 4).fg('white')
p.text(() => OS.clock('HH:mm:ss')).at('center', 6).fg('amber')
p.text('press letters; q quits host').at('center', 9).fg('dim')
p.text(() => st.last ? 'last key: ' + st.last : '').at('center', 10)
p.gauge().at(4, 13).label('batt').value(() => OS.battery).width(16).showPct()

app.on('tick', 1000, () => st.n++)
app.key('a', (m, k) => { st.last = k })
app.key('b', (m, k) => { st.last = k })
app.statusbar('native host: arrows/type keys · q exits')
app.mount()
