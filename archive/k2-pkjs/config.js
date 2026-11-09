// Shack telemetry daemon (Tailscale). Override via phone WebView localStorage key k2_shack_url.
module.exports = {
  DEFAULT_SHACK_URL: 'http://100.108.191.85:8772/api/v1/state',
  POLL_INTERVAL_MS: 2000,
  XHR_TIMEOUT_MS: 8000
};
