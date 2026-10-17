module.exports = [
  { "type": "heading", "defaultValue": "CW Messenger" },
  { "type": "text", "defaultValue": "Changes to CW timing apply on the next playback or keying session. Notification reading is experimental and only operates while CW Messenger is open." },
  { "type": "section", "items": [
    { "type": "heading", "defaultValue": "Incoming SMS via Pebble (Preview)" },
    { "type": "text", "defaultValue": "No Android helper needed. Requires Pebble mobile app 1.15+, experimental plugins enabled, and Notifications permission. Only works while CW Messenger is open." },
    { "type": "toggle", "messageKey": "NotifEnabled", "label": "Read new message notifications as CW", "defaultValue": false },
    { "type": "input", "messageKey": "NotifApps", "label": "Allowed SMS app names (comma-separated)", "defaultValue": "Messages", "attributes": { "maxlength": 96 }, "description": "Must match the app name in the Android notification shade, e.g. Messages. Other apps are ignored." }
  ]},
  { "type": "section", "items": [
    { "type": "heading", "defaultValue": "CW Timing and Sound" },
    { "type": "slider", "messageKey": "Wpm", "label": "Speed (WPM)", "defaultValue": 20, "min": 5, "max": 40, "step": 1 },
    { "type": "slider", "messageKey": "ToneHz", "label": "Tone (Hz)", "defaultValue": 600, "min": 200, "max": 1500, "step": 25 },
    { "type": "slider", "messageKey": "Volume", "label": "Speaker volume", "defaultValue": 60, "min": 0, "max": 100, "step": 5 },
    { "type": "slider", "messageKey": "SyncMs", "label": "Receive haptic offset (ms)", "defaultValue": 80, "min": -300, "max": 400, "step": 10, "description": "Keep at 80 if playback is already synchronized." }
  ]},
  { "type": "section", "items": [
    { "type": "heading", "defaultValue": "Receiving" },
    { "type": "toggle", "messageKey": "RxAudio", "label": "Play received CW through speaker", "defaultValue": true },
    { "type": "toggle", "messageKey": "RxVibe", "label": "Vibrate received CW", "defaultValue": true },
    { "type": "toggle", "messageKey": "AutoPlay", "label": "Automatically play a received message", "defaultValue": true }
  ]},
  { "type": "section", "items": [
    { "type": "heading", "defaultValue": "Sending / Keyer" },
    { "type": "toggle", "messageKey": "TxAudio", "label": "CW sidetone", "defaultValue": true },
    { "type": "toggle", "messageKey": "TxVibe", "label": "Vibrate while keying", "defaultValue": true },
    { "type": "toggle", "messageKey": "ReversePaddles", "label": "Swap Up/Down dit and dah", "defaultValue": false },
    { "type": "slider", "messageKey": "StraightThreshold", "label": "Straight-key dah threshold (% of dit)", "defaultValue": 200, "min": 125, "max": 300, "step": 25 }
  ]},
  { "type": "section", "items": [
    { "type": "heading", "defaultValue": "App Startup" },
    { "type": "select", "messageKey": "StartMode", "defaultValue": "0", "label": "Initial screen", "serializeValueAs": "integer",
      "options": [
        { "label": "Send / CW practice", "value": "0" },
        { "label": "Receive / read CW", "value": "1" }
      ]
    }
  ]},
  { "type": "section", "items": [
    { "type": "heading", "defaultValue": "Demo Message" },
    { "type": "input", "messageKey": "DemoSender", "defaultValue": "Ben", "label": "Sample sender", "attributes": { "maxlength": 31 } },
    { "type": "input", "messageKey": "DemoMessage", "defaultValue": "good night bens best", "label": "Sample message", "attributes": { "maxlength": 120 }, "description": "This is the fallback demo. SMS preview uses notifications only while the watchapp is open." }
  ]},
  { "type": "submit", "defaultValue": "Save CW Settings" }
];
