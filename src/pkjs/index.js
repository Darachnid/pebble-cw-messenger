// Pebble Time 2 CW Messenger: no separate Android APK.
// Clay owns the Settings gear; the current Android Pebble app (1.15+)
// owns the experimental built-in notifications/notification source.
var Clay = require('@rebble/clay');
var clayConfig = require('./config');
var keys = require('message_keys');
var clay = new Clay(clayConfig);

var subscription = null;
var initialized = false;
var seen = {};
var seenOrder = [];
var outgoing = [];
var sending = false;
var ready = false;

function setting(name, fallback) {
  try {
    var values = clay.getSettings();
    var value = values && values[keys[name]];
    return value === undefined || value === null ? fallback : value;
  } catch (e) {
    return fallback;
  }
}

function allowApp(appName) {
  var configured = String(setting('NotifApps', 'Messages'));
  var name = String(appName || '').toLowerCase().trim();
  if (!name) return false;
  var list = configured.split(',');
  for (var i = 0; i < list.length; ++i) {
    if (list[i].toLowerCase().trim() === name) return true;
  }
  return false;
}

function cleanMorseText(value, maximum) {
  var input = String(value || '');
  if (typeof input.normalize === 'function') {
    try { input = input.normalize('NFKD'); } catch (_) {}
  }
  // The Pebble watch's Morse encoder only supports the following characters.
  var cleaned = input.toUpperCase()
    .replace(/[\u0300-\u036f]/g, '')
    .replace(/[^A-Z0-9 .,?\/:=+@'\-]+/g, ' ')
    .replace(/\s+/g, ' ').trim();
  if (cleaned.length > maximum) return cleaned.slice(0, maximum - 3) + '...';
  return cleaned;
}

function propertyText(properties, name) {
  var prop = properties && properties[name];
  return (prop && prop.longText && prop.longText.text) ||
         (prop && prop.shortText && prop.shortText.text) || '';
}

function enqueueNotification(sender, body) {
  var title = cleanMorseText(sender, 31);
  var message = cleanMorseText(body, 120);
  if (!title || !message) return;
  // Notifications are private: never print their sender or message to logs.
  outgoing.push({ SmsSender: title, SmsBody: message });
  // Avoid flooding AppMessage when several shade events arrive at once.
  if (outgoing.length > 5) outgoing.shift();
  sendNext();
}

function sendNext() {
  if (sending || !outgoing.length || !ready) return;
  sending = true;
  Pebble.sendAppMessage(outgoing[0], function() {
    outgoing.shift();
    sending = false;
    sendNext();
  }, function() {
    // Do not retry indefinitely or log notification contents.
    outgoing.shift();
    sending = false;
    sendNext();
  });
}

function acceptSnapshot(envelope) {
  var instances = envelope && envelope.instances;
  if (!instances || !Array.isArray(instances)) return;
  // Pebble's built-in source lists the current SHADE, not only new events.
  // Prime the dedup cache without speaking existing/old notifications.
  var recent = [];
  for (var i = 0; i < instances.length; ++i) {
    var inst = instances[i];
    if (!inst || !inst.instanceId) continue;
    var p = inst.properties || {};
    var title = propertyText(p, 'name');
    var body = propertyText(p, 'body');
    var app = propertyText(p, 'app');
    var signature = String(inst.instanceId) + '|' + String(title) + '|' + String(body);
    var fresh = !Object.prototype.hasOwnProperty.call(seen, signature);
    seen[signature] = true;
    seenOrder.push(signature);
    if (initialized && fresh && body && allowApp(app)) {
      recent.push({title:title, body:body});
    }
  }
  initialized = true;
  while (seenOrder.length > 300) {
    var old = seenOrder.shift();
    delete seen[old];
  }
  // Newest notifications are returned first. Forward oldest first.
  for (var j = recent.length - 1; j >= 0; --j) {
    enqueueNotification(recent[j].title, recent[j].body);
  }
}

function updateSubscription() {
  if (!ready) return;
  if (subscription) {
    try { subscription.unsubscribe(); } catch (_) {}
    subscription = null;
  }
  initialized = false;
  seen = {};
  seenOrder = [];
  outgoing = [];
  if (Number(setting('NotifEnabled', 0)) !== 1) return;
  if (typeof Pebble.subscribeToSource !== 'function') {
    console.log('Notification source unavailable: enable experimental plugins in Pebble app');
    return;
  }
  try {
    subscription = Pebble.subscribeToSource({
      category: 'notifications',
      item: 'notification',
      properties: ['name', 'body', 'app'],
      onData: acceptSnapshot,
      onError: function(err) {
        // Error codes only: never log SMS metadata.
        console.log('Notification source: ' + (err && err.code || 'UNKNOWN'));
      }
    });
  } catch (e) {
    console.log('Notification source subscription unsupported');
  }
}

Pebble.addEventListener('ready', function() {
  ready = true;
  updateSubscription();
});

// Clay handles saving first (it registered its handler in the constructor).
Pebble.addEventListener('webviewclosed', function() {
  setTimeout(updateSubscription, 250);
});
