// Pebble-supported offline settings: no hosted web page required.
// Clay handles the Pebble "Settings" gear, persistence in phone localStorage
// and AppMessage delivery to the watch when Save is tapped.
var Clay = require('@rebble/clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig);
Pebble.addEventListener('ready', function() {
  console.log('CW Messenger settings available');
});
