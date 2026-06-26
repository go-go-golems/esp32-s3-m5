// fetch-healthz.js — firmware fetch smoke for AtomS3R QuickJS.
// Edit DEVICE_IP if DHCP changes, then run from /scripts/fetch-healthz.js.

var DEVICE_IP = '192.168.4.22';
var url = 'http://' + DEVICE_IP + '/healthz';

fetch(url)
  .then(function(resp) {
    print('fetch status=' + resp.status + ' ok=' + resp.ok + ' url=' + resp.url);
    return resp.text();
  })
  .then(function(body) {
    print('fetch body=' + body);
  });
