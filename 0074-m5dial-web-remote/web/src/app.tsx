import { FormEvent, useEffect, useMemo, useRef, useState } from 'react'
import { useStore } from './store'

const items = Array.from({ length: 120 }, (_, index) => ({
  id: `item-${index + 1}`,
  label: `List item ${String(index + 1).padStart(3, '0')}`,
  detail: index % 3 === 0 ? 'Debug waypoint' : index % 3 === 1 ? 'UI sample row' : 'Remote-controlled row',
}))

function formatTime(iso: string) {
  if (!iso) return '-'
  return new Date(iso).toLocaleTimeString()
}

export function App() {
  const wsConnected = useStore((state) => state.wsConnected)
  const connectWs = useStore((state) => state.connectWs)
  const devices = useStore((state) => state.devices)
  const history = useStore((state) => state.history)
  const wsFrames = useStore((state) => state.wsFrames)
  const selectedDeviceId = useStore((state) => state.selectedDeviceId)
  const selectDevice = useStore((state) => state.selectDevice)
  const clearFrames = useStore((state) => state.clearFrames)
  const sendUiCommand = useStore((state) => state.sendUiCommand)
  const lastCommandFeedback = useStore((state) => state.lastCommandFeedback)

  const [messageText, setMessageText] = useState('Hello from React')
  const [positionValue, setPositionValue] = useState('12')

  useEffect(() => {
    connectWs()
  }, [connectWs])

  const selectedDevice = useMemo(
    () => devices.find((device) => device.device_id === selectedDeviceId) ?? devices[0] ?? null,
    [devices, selectedDeviceId],
  )

  const selectedIndex = useMemo(() => {
    if (!selectedDevice) return 0
    const count = items.length
    const raw = selectedDevice.position % count
    return raw < 0 ? raw + count : raw
  }, [selectedDevice])

  const selectedRowRef = useRef<HTMLDivElement | null>(null)
  const frameLogRef = useRef<HTMLDivElement | null>(null)

  useEffect(() => {
    selectedRowRef.current?.scrollIntoView({ block: 'center', behavior: 'smooth' })
  }, [selectedIndex])

  useEffect(() => {
    const element = frameLogRef.current
    if (!element) return
    const nearBottom = element.scrollTop + element.clientHeight >= element.scrollHeight - 48
    if (nearBottom) {
      element.scrollTop = element.scrollHeight
    }
  }, [wsFrames.length])

  const controlsDisabled = !selectedDevice || !wsConnected

  function handleMessageSubmit(event: FormEvent<HTMLFormElement>) {
    event.preventDefault()
    sendUiCommand(selectedDevice?.device_id ?? '', 'show_message', { text: messageText })
  }

  function handlePositionSubmit(event: FormEvent<HTMLFormElement>) {
    event.preventDefault()
    const parsed = Number.parseInt(positionValue, 10)
    if (Number.isNaN(parsed)) {
      return
    }
    sendUiCommand(selectedDevice?.device_id ?? '', 'set_position', { value: parsed })
  }

  return (
    <main className="shell">
      <section className="hero panel reveal">
        <div>
          <p className="eyebrow">M5Dial Browser Remote</p>
          <h1>Physical knob in the loop, browser widget in control.</h1>
          <p className="lede">
            The browser listens to the Go server on <code>/ws/browser</code>, the dial pushes encoder and button events
            to <code>/ws/device</code>, and the browser can now send <code>ui_command</code> frames back to the dial
            through the same server.
          </p>
        </div>
        <div className="statusRail">
          <span className={`pill ${wsConnected ? 'ok' : 'bad'}`}>{wsConnected ? 'Browser WS connected' : 'Browser WS disconnected'}</span>
          <span className="pill neutral">{devices.length} device{devices.length === 1 ? '' : 's'}</span>
          {selectedDevice ? <span className="pill neutral">Selected: {selectedDevice.device_id}</span> : null}
        </div>
      </section>

      <section className="grid">
        <div className="panel reveal">
          <div className="panelHead">
            <div>
              <p className="eyebrow">Live List</p>
              <h2>Scroll position mirrors the dial</h2>
            </div>
            <div className="indexBadge">#{selectedIndex + 1}</div>
          </div>
          <div className="listSurface">
            {items.map((item, index) => (
              <div
                key={item.id}
                ref={index === selectedIndex ? selectedRowRef : null}
                className={`listRow ${index === selectedIndex ? 'active' : ''}`}
              >
                <div>
                  <div className="rowLabel">{item.label}</div>
                  <div className="rowDetail">{item.detail}</div>
                </div>
                <div className="rowIndex">{index + 1}</div>
              </div>
            ))}
          </div>
        </div>

        <div className="stack">
          <section className="panel reveal delay1">
            <div className="panelHead">
              <div>
                <p className="eyebrow">Device State</p>
                <h2>Current device summary</h2>
              </div>
            </div>
            <div className="devicePicker">
              {devices.length === 0 ? <div className="empty">No device connected yet.</div> : null}
              {devices.map((device) => (
                <button
                  key={device.device_id}
                  className={`deviceChip ${selectedDevice?.device_id === device.device_id ? 'selected' : ''}`}
                  onClick={() => selectDevice(device.device_id)}
                >
                  {device.device_id}
                </button>
              ))}
            </div>
            {selectedDevice ? (
              <dl className="kv">
                <div><dt>Connected</dt><dd>{selectedDevice.connected ? 'yes' : 'no'}</dd></div>
                <div><dt>Position</dt><dd>{selectedDevice.position}</dd></div>
                <div><dt>Last delta</dt><dd>{selectedDevice.last_delta}</dd></div>
                <div><dt>Last button</dt><dd>{selectedDevice.last_button ?? '-'}</dd></div>
                <div><dt>Last type</dt><dd>{selectedDevice.last_type || '-'}</dd></div>
                <div><dt>Last command</dt><dd>{selectedDevice.last_command ?? '-'}</dd></div>
                <div><dt>Command status</dt><dd>{selectedDevice.last_command_status ?? '-'}</dd></div>
                <div><dt>Device text</dt><dd>{selectedDevice.last_text ?? '-'}</dd></div>
                <div><dt>Last request</dt><dd>{selectedDevice.last_request_id ?? '-'}</dd></div>
                <div><dt>Last sequence</dt><dd>{selectedDevice.last_seq}</dd></div>
                <div><dt>RX count</dt><dd>{selectedDevice.rx_count}</dd></div>
                <div><dt>Last seen</dt><dd>{formatTime(selectedDevice.last_seen)}</dd></div>
              </dl>
            ) : null}
          </section>

          <section className="panel reveal delay2">
            <div className="panelHead">
              <div>
                <p className="eyebrow">Dial Commands</p>
                <h2>Send browser commands back to the ESP32</h2>
              </div>
            </div>

            <div className="commandGrid">
              <form className="commandCard" onSubmit={handleMessageSubmit}>
                <label className="fieldLabel" htmlFor="messageText">
                  Show message on dial
                </label>
                <input
                  id="messageText"
                  className="textInput"
                  value={messageText}
                  onChange={(event) => setMessageText(event.target.value)}
                  placeholder="Dial message"
                  maxLength={60}
                />
                <button className="actionButton" disabled={controlsDisabled}>
                  Send message
                </button>
              </form>

              <form className="commandCard" onSubmit={handlePositionSubmit}>
                <label className="fieldLabel" htmlFor="positionValue">
                  Force knob position
                </label>
                <input
                  id="positionValue"
                  className="textInput"
                  type="number"
                  inputMode="numeric"
                  value={positionValue}
                  onChange={(event) => setPositionValue(event.target.value)}
                  placeholder="0"
                />
                <button className="actionButton" disabled={controlsDisabled}>
                  Set position
                </button>
              </form>
            </div>

            <div className={`commandFeedback ${lastCommandFeedback?.status.startsWith('ack') ? 'ok' : lastCommandFeedback?.status === 'rejected' ? 'bad' : 'neutral'}`}>
              {lastCommandFeedback ? (
                <>
                  <strong>
                    {lastCommandFeedback.status} #{lastCommandFeedback.requestId}
                  </strong>
                  <span>
                    {lastCommandFeedback.command} on {lastCommandFeedback.deviceId || 'no-device'}: {lastCommandFeedback.detail}
                  </span>
                </>
              ) : (
                <span>Commands will show immediate queue/reject status and later device acknowledgements here.</span>
              )}
            </div>
          </section>

          <section className="panel reveal delay3">
            <div className="panelHead">
              <div>
                <p className="eyebrow">Server History</p>
                <h2>Recent device messages</h2>
              </div>
            </div>
            <div className="historyList">
              {history.slice().reverse().slice(0, 10).map((entry) => (
                <div className="historyRow" key={entry.id}>
                  <span>{entry.device_id}</span>
                  <span>{entry.type}</span>
                  <span>{formatTime(entry.received_at)}</span>
                </div>
              ))}
              {history.length === 0 ? <div className="empty">History will appear once the dial sends frames.</div> : null}
            </div>
          </section>
        </div>
      </section>

      <section className="panel reveal delay3">
        <div className="panelHead">
          <div>
            <p className="eyebrow">Debug Frames</p>
            <h2>Browser WebSocket frame log</h2>
          </div>
          <button className="clearButton" onClick={() => clearFrames()}>
            Clear log
          </button>
        </div>
        <div className="frameLog" ref={frameLogRef}>
          {wsFrames.map((frame) => (
            <div className="frameRow" key={frame.id}>
              <span>{new Date(frame.rxMs).toLocaleTimeString()}</span>
              <code>{frame.parseError ? frame.parseError : frame.raw}</code>
            </div>
          ))}
          {wsFrames.length === 0 ? <div className="empty">Waiting for websocket frames from the server.</div> : null}
        </div>
      </section>
    </main>
  )
}
