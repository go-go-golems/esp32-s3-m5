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

function parseArgs(argv) {
  const opts = {
    broker: 'mqtt://localhost:1884',
    baseTopic: 'zigbee2mqtt',
    seconds: 120,
    device: '',
    timeout: '60s',
  };

  let positional = [];
  for (const arg of argv) {
    const idx = arg.indexOf('=');
    if (idx > 0) {
      const key = arg.slice(0, idx);
      const value = arg.slice(idx + 1);
      switch (key) {
        case 'broker':
          opts.broker = value;
          break;
        case 'baseTopic':
          opts.baseTopic = value;
          break;
        case 'seconds':
          opts.seconds = parseInt(value, 10);
          break;
        case 'device':
          opts.device = value;
          break;
        case 'timeout':
          opts.timeout = value;
          break;
        default:
          break;
      }
    } else {
      positional.push(arg);
    }
  }

  if (positional.length > 0) {
    opts.broker = positional[0] || opts.broker;
  }
  if (positional.length > 1) {
    opts.baseTopic = positional[1] || opts.baseTopic;
  }
  if (positional.length > 2) {
    opts.seconds = parseInt(positional[2], 10);
  }
  if (positional.length > 3) {
    opts.device = positional[3];
  }
  if (positional.length > 4) {
    opts.timeout = positional[4];
  }

  if (!opts.seconds || Number.isNaN(opts.seconds)) {
    opts.seconds = 120;
  }

  return opts;
}

const opts = parseArgs(args);

const client = zigctl.connect({
  broker: opts.broker,
  baseTopic: opts.baseTopic,
  qos: 0,
  timeout: opts.timeout,
  debug: true,
});

const stream = client.watch({
  topics: ['bridge/event'],
  duration: String(opts.seconds) + 's',
});

const permit = client.permitJoin({ seconds: opts.seconds, device: opts.device });
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
