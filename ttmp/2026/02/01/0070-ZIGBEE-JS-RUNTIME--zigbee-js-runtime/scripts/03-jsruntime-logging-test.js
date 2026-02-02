const zigctl = require('zigctl');

const args = (typeof zigctlArgs !== 'undefined' && zigctlArgs) ? zigctlArgs : [];
const broker = args[0] || 'mqtt://localhost:1884';
const baseTopic = args[1] || 'zigbee2mqtt';

const client = zigctl.connect({
  broker: broker,
  baseTopic: baseTopic,
  qos: 0,
  timeout: '10s',
  debug: true,
});

console.log('bridgeInfo:', JSON.stringify(client.bridgeInfo()));
console.log('devices:', JSON.stringify(client.devices()));

const stream = client.watch({ topics: ['bridge/event'], duration: '5s' });

while (true) {
  const next = stream.next();
  if (next.done) {
    break;
  }
  console.log('event:', JSON.stringify(next.value));
}

client.close();
