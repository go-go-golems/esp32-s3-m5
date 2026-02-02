const zigctl = require('zigctl');

const client = zigctl.connect({
  broker: 'mqtt://localhost:1884',
  baseTopic: 'zigbee2mqtt',
  qos: 0,
  timeout: '10s',
});

const stream = client.watch({
  topics: ['bridge/event'],
  duration: '60s',
});

client.permitJoin({ seconds: 60 });

while (true) {
  const next = stream.next();
  if (next.done) {
    break;
  }
  console.log(JSON.stringify(next.value));
}

client.close();
