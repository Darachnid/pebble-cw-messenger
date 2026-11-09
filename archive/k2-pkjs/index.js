var config = require('./config');

var pollTimer = null;
var sendQueue = [];
var sending = false;

function shackUrl() {
  try {
    var stored = localStorage.getItem('k2_shack_url');
    if (stored && stored.length > 0) {
      return stored;
    }
  } catch (e) {
    console.log('localStorage unavailable: ' + e);
  }
  return config.DEFAULT_SHACK_URL;
}

function connectionCode(connection) {
  if (connection === 'LIVE') {
    return 1;
  }
  if (connection === 'STALE') {
    return 2;
  }
  return 0;
}

function audioFilterCode(label) {
  if (!label || label === 'OFF') {
    return 0;
  }
  if (label === 'AF2') {
    return 2;
  }
  if (label === 'AF1') {
    return 1;
  }
  return 0;
}

function nbCode(label) {
  if (!label || label === 'OFF') {
    return 0;
  }
  if (label === 'NB2') {
    return 2;
  }
  if (label === 'NB1') {
    return 1;
  }
  return 0;
}

function payloadFromState(data) {
  var conn = connectionCode(data.connection);
  var ritHz = typeof data.rit_offset_hz === 'number' ? data.rit_offset_hz : 0;
  var msg = {
    conn: conn,
    freq: data.frequency_text || '--',
    mode: data.mode_text || '--',
    band: data.band_text || '--',
    xtal_bw: data.crystal_bw_hz || 0,
    audio_f: audioFilterCode(data.audio_filter),
    s_seg: typeof data.s_meter_segments === 'number' ? data.s_meter_segments : 0,
    s_lbl: data.s_meter_label || '',
    rit_on: data.rit_enabled ? 1 : 0,
    rit_off: Math.round(ritHz / 10),
    agc: data.agc || '--',
    nb: nbCode(data.noise_blanker)
  };

  if (data.transmitting && data.power_segments !== null && data.power_segments !== undefined) {
    msg.pwr_valid = 1;
    msg.pwr_seg = data.power_segments;
    msg.pwr_lbl = data.power_label || '';
  } else {
    msg.pwr_valid = 0;
  }
  return msg;
}

function offlinePayload(reason) {
  console.log('shack offline: ' + reason);
  return {
    conn: 0,
    freq: '--',
    mode: '--',
    band: '--',
    xtal_bw: 0,
    audio_f: 0,
    s_seg: 0,
    s_lbl: '',
    pwr_valid: 0,
    rit_on: 0,
    rit_off: 0,
    agc: '--',
    nb: 0
  };
}

function flushSendQueue() {
  if (sending || sendQueue.length === 0) {
    return;
  }
  sending = true;
  var next = sendQueue.shift();
  Pebble.sendAppMessage(
    next,
    function () {
      sending = false;
      flushSendQueue();
    },
    function (e) {
      console.log('sendAppMessage failed: ' + JSON.stringify(e));
      sending = false;
      flushSendQueue();
    }
  );
}

function pushToWatch(dict) {
  sendQueue.push(dict);
  flushSendQueue();
}

function pollShack() {
  var url = shackUrl();
  var xhr = new XMLHttpRequest();
  xhr.timeout = config.XHR_TIMEOUT_MS;
  xhr.open('GET', url, true);
  xhr.onload = function () {
    if (xhr.status >= 200 && xhr.status < 300) {
      try {
        var data = JSON.parse(xhr.responseText);
        pushToWatch(payloadFromState(data));
      } catch (parseErr) {
        pushToWatch(offlinePayload('json parse'));
      }
    } else {
      pushToWatch(offlinePayload('http ' + xhr.status));
    }
  };
  xhr.onerror = function () {
    pushToWatch(offlinePayload('network'));
  };
  xhr.ontimeout = function () {
    pushToWatch(offlinePayload('timeout'));
  };
  xhr.send();
}

function startPolling() {
  if (pollTimer) {
    clearInterval(pollTimer);
  }
  pollShack();
  pollTimer = setInterval(pollShack, config.POLL_INTERVAL_MS);
}

Pebble.addEventListener('ready', function () {
  console.log('K2 PKJS ready; shack ' + shackUrl());
  startPolling();
});

Pebble.addEventListener('appmessage', function (e) {
  if (e.payload && e.payload.resync) {
    pollShack();
  }
});
