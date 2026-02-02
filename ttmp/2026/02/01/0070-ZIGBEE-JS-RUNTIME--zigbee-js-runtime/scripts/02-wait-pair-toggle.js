const zigctl = require('zigctl');
const exec = require('exec');

function sleep(seconds) {
  exec.run('sleep', [String(seconds)]);
}

const args = (typeof zigctlArgs !== 'undefined' && zigctlArgs) ? zigctlArgs : [];
const broker = args[0] || 'mqtt://localhost:1884';
const baseTopic = args[1] || 'zigbee2mqtt';
const targetDevice = args[2] || '';

const client = zigctl.connect({
  broker: broker,
  baseTopic: baseTopic,
  qos: 0,
  timeout: '10s',
});

const stream = client.watch({
  topics: ['bridge/event'],
});

let device = '';

while (true) {
  const next = stream.next();
  if (next.done) {
    throw new Error('watch ended before a device joined');
  }

  const payload = next.value && next.value.payload ? next.value.payload : null;
  if (!payload || !payload.type || !payload.data) {
    continue;
  }

  const name = payload.data.friendly_name || payload.data.ieee_address || '';
  if (!name) {
    continue;
  }

  if (targetDevice && name !== targetDevice) {
    continue;
  }

  device = targetDevice || name;
  break;
}

stream.stop();

let state = 'ON';
while (true) {
  client.publish(`${device}/set`, { state: state });
  state = state === 'ON' ? 'OFF' : 'ON';
  sleep(5);
}
