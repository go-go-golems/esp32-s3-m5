const zigctl = require('zigctl');

function scalar(value) {
  if (value === null || value === undefined) {
    return 'null';
  }
  if (typeof value === 'string') {
    return JSON.stringify(value);
  }
  if (typeof value === 'number' || typeof value === 'boolean') {
    return String(value);
  }
  return JSON.stringify(value);
}

function toYaml(value, indent) {
  const pad = '  '.repeat(indent);

  if (Array.isArray(value)) {
    if (value.length === 0) {
      return pad + '[]';
    }
    return value.map((item) => {
      if (item !== null && typeof item === 'object') {
        return pad + '-\n' + toYaml(item, indent + 1);
      }
      return pad + '- ' + scalar(item);
    }).join('\n');
  }

  if (value !== null && typeof value === 'object') {
    const lines = [];
    for (const key of Object.keys(value)) {
      const item = value[key];
      if (item !== null && typeof item === 'object') {
        lines.push(pad + key + ':');
        lines.push(toYaml(item, indent + 1));
      } else {
        lines.push(pad + key + ': ' + scalar(item));
      }
    }
    if (lines.length === 0) {
      return pad + '{}';
    }
    return lines.join('\n');
  }

  return pad + scalar(value);
}

const args = (typeof zigctlArgs !== 'undefined' && zigctlArgs) ? zigctlArgs : [];
const broker = args[0] || 'mqtt://localhost:1884';
const baseTopic = args[1] || 'zigbee2mqtt';
const seconds = args[2] ? parseInt(args[2], 10) : 120;
const targetDevice = args[3] || '';

const client = zigctl.connect({
  broker: broker,
  baseTopic: baseTopic,
  qos: 0,
  timeout: '10s',
  debug: true,
});

const stream = client.watch({
  topics: ['bridge/event'],
  duration: String(seconds) + 's',
});

const permit = client.permitJoin({ seconds: seconds, device: targetDevice });
console.log('---');
console.log(toYaml({ permit_join: permit }, 0));

while (true) {
  const next = stream.next();
  if (next.done) {
    break;
  }
  console.log('---');
  console.log(toYaml(next.value, 0));
}

client.close();
