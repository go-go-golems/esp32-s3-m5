const zigctl = require('zigctl');

const client = zigctl.connect({
  broker: 'mqtt://localhost:1884',
  baseTopic: 'zigbee2mqtt',
  qos: 0,
  timeout: '10s',
});

client.publish('office_plug/set', { state: 'ON' });
client.publish('office_plug/set', { countdown_to_turn_off: 300 });
client.publish('office_plug/set', { power_on_behavior: 'previous' });

client.close();
