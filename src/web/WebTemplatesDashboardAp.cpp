// SPDX-FileCopyrightText: 2025-2026 Volodymyr Papush (21CNCStudio)
// SPDX-License-Identifier: GPL-3.0-or-later
// GPL-3.0-or-later: https://www.gnu.org/licenses/gpl-3.0.html
// Want to use this code in a commercial product while keeping modifications proprietary?
// Purchase a Commercial License: see COMMERCIAL_LICENSE_SUMMARY.md

#include "web/WebTemplates.h"

namespace WebTemplates {

const char kDashboardPageTemplateAp[] PROGMEM = R"HTML_DASH_AP(
<!doctype html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>Aura Dashboard (AP Offline)</title>
  <style>
    :root {
      --bg: #0b1020;
      --panel: #121a2f;
      --panel-2: #0f1629;
      --text: #e8edf7;
      --muted: #9aa7c0;
      --accent: #3dd6c6;
      --accent-2: #2ea899;
      --warn: #f59e0b;
      --danger: #ef4444;
      --ok: #22c55e;
      --border: #24314f;
      --line: #2f416d;
    }
    * { box-sizing: border-box; }
    body {
      margin: 0;
      padding: 16px;
      background: radial-gradient(1200px 600px at 100% -20%, #17244a, var(--bg));
      color: var(--text);
      font-family: "Segoe UI", Roboto, Arial, sans-serif;
    }
    .wrap {
      max-width: 1180px;
      margin: 0 auto;
    }
    .top {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
      margin-bottom: 14px;
      flex-wrap: wrap;
    }
    .top-right {
      display: flex;
      align-items: center;
      gap: 10px;
      flex-wrap: wrap;
      justify-content: flex-end;
    }
    .brand {
      display: flex;
      align-items: center;
      gap: 8px;
    }
    .brand-dot {
      width: 10px;
      height: 10px;
      border-radius: 999px;
      background: #34d399;
      box-shadow: 0 0 8px #34d399;
      flex: 0 0 auto;
    }
    .title {
      font-size: 22px;
      font-weight: 700;
      letter-spacing: 0.02em;
    }
    .subtitle {
      font-size: 12px;
      color: var(--muted);
      margin-top: 2px;
    }
    .actions {
      display: flex;
      gap: 8px;
      flex-wrap: wrap;
    }
    .clock {
      text-align: right;
      min-width: 110px;
    }
    .clock-time {
      font-size: 22px;
      font-weight: 700;
      line-height: 1;
      letter-spacing: 0.03em;
    }
    .clock-date {
      margin-top: 3px;
      color: var(--muted);
      font-size: 11px;
      font-weight: 600;
      letter-spacing: 0.08em;
      text-transform: uppercase;
    }
    button, .link-btn, select, input[type="number"], input[type="text"] {
      border: 1px solid var(--border);
      background: var(--panel);
      color: var(--text);
      border-radius: 10px;
      padding: 9px 12px;
      font-size: 13px;
      text-decoration: none;
    }
    button, .link-btn {
      cursor: pointer;
    }
    button:hover, .link-btn:hover {
      border-color: #37518a;
    }
    button.primary {
      background: linear-gradient(180deg, var(--accent), var(--accent-2));
      color: #052423;
      border-color: transparent;
      font-weight: 700;
    }
    button.danger-btn {
      background: #271515;
      border-color: #7a2c2c;
      color: #ffd7d7;
    }
    button:disabled {
      opacity: 0.6;
      cursor: not-allowed;
    }
    select, input[type="number"], input[type="text"] {
      width: 100%;
      cursor: default;
    }
    input[type="checkbox"] {
      accent-color: var(--accent);
      width: 16px;
      height: 16px;
    }
    .status {
      border: 1px solid #355f8e;
      background: #0e233d;
      color: #b8d7ff;
      border-radius: 10px;
      padding: 10px 12px;
      margin-bottom: 14px;
      font-size: 13px;
      line-height: 1.4;
    }
    .status.warn {
      border-color: #7f5a1e;
      background: #2a2110;
      color: #ffd89a;
    }
    .status.ok {
      border-color: #1f6a3c;
      background: #102317;
      color: #b8ffcb;
    }
    .tabs {
      display: flex;
      flex-wrap: wrap;
      gap: 8px;
      margin-bottom: 12px;
    }
    .tab-btn {
      border: 1px solid var(--border);
      background: #0f1930;
      color: var(--muted);
      border-radius: 999px;
      padding: 7px 12px;
      font-size: 12px;
      text-transform: uppercase;
      letter-spacing: 0.08em;
    }
    .tab-btn.active {
      color: #092927;
      border-color: transparent;
      background: linear-gradient(180deg, var(--accent), var(--accent-2));
      font-weight: 700;
    }
    .tab-panel {
      display: none;
    }
    .tab-panel.active {
      display: block;
    }
    .sec {
      background: linear-gradient(180deg, var(--panel), var(--panel-2));
      border: 1px solid var(--border);
      border-radius: 12px;
      padding: 12px;
      margin-bottom: 12px;
    }
    .sec h3 {
      margin: 0 0 10px;
      font-size: 14px;
      color: var(--muted);
      text-transform: uppercase;
      letter-spacing: 0.08em;
      font-weight: 700;
    }
    .sec p.note {
      margin: 0;
      color: var(--muted);
      font-size: 12px;
      line-height: 1.35;
    }
    .grid {
      display: grid;
      grid-template-columns: repeat(auto-fill, minmax(150px, 1fr));
      gap: 10px;
    }
    .card {
      background: linear-gradient(180deg, #111c34, #0d152a);
      border: 1px solid var(--line);
      border-radius: 12px;
      padding: 10px;
      min-height: 84px;
    }
    .card .k {
      color: var(--muted);
      font-size: 11px;
      text-transform: uppercase;
      letter-spacing: 0.08em;
    }
    .card .v {
      margin-top: 6px;
      font-size: 22px;
      font-weight: 700;
      line-height: 1.1;
    }
    .card .u {
      color: var(--muted);
      font-size: 12px;
      margin-left: 5px;
    }
    .meta {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(190px, 1fr));
      gap: 6px 12px;
      font-size: 13px;
    }
    .meta b {
      color: var(--muted);
      font-weight: 600;
      margin-right: 6px;
    }
    .row {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
      gap: 10px;
      align-items: end;
    }
    .field {
      display: flex;
      flex-direction: column;
      gap: 6px;
    }
    .field label {
      color: var(--muted);
      font-size: 12px;
    }
    .field.inline {
      flex-direction: row;
      align-items: center;
      gap: 8px;
      padding-top: 22px;
    }
    .btn-row {
      display: flex;
      gap: 8px;
      flex-wrap: wrap;
      margin-top: 10px;
    }
    .seg {
      display: inline-flex;
      border: 1px solid var(--line);
      border-radius: 10px;
      overflow: hidden;
      background: #0d162c;
    }
    .seg-btn {
      border: 0;
      border-right: 1px solid var(--line);
      border-radius: 0;
      background: transparent;
      color: var(--muted);
      padding: 8px 12px;
      font-size: 12px;
      font-weight: 700;
      letter-spacing: 0.06em;
      text-transform: uppercase;
      cursor: pointer;
    }
    .seg-btn:last-child {
      border-right: 0;
    }
    .seg-btn.active {
      color: #082321;
      background: linear-gradient(180deg, var(--accent), var(--accent-2));
    }
    .charts {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
      gap: 10px;
    }
    .chart-box {
      border: 1px solid var(--line);
      border-radius: 10px;
      background: #0c1428;
      padding: 8px;
    }
    .chart-head {
      display: flex;
      justify-content: space-between;
      align-items: baseline;
      margin-bottom: 6px;
      gap: 8px;
    }
    .chart-name {
      font-size: 13px;
      font-weight: 600;
    }
    .chart-latest {
      font-size: 12px;
      color: var(--muted);
      text-align: right;
    }
    svg {
      width: 100%;
      height: 86px;
      display: block;
      border-radius: 8px;
      background: #0a1223;
    }
    .events {
      display: grid;
      gap: 7px;
    }
    .ev {
      border: 1px solid var(--line);
      border-radius: 8px;
      background: #0c1428;
      padding: 8px 10px;
      font-size: 13px;
    }
    .ev-top {
      display: flex;
      justify-content: space-between;
      gap: 10px;
      color: var(--muted);
      font-size: 11px;
      margin-bottom: 4px;
    }
    .ev-msg {
      line-height: 1.35;
      word-break: break-word;
    }
    .ev.critical { border-color: #7f1d1d; }
    .ev.warning { border-color: #7f5a1e; }
    .progress-wrap {
      margin-top: 8px;
      height: 8px;
      border-radius: 999px;
      overflow: hidden;
      border: 1px solid var(--line);
      background: #0c1428;
    }
    .progress {
      height: 100%;
      width: 0%;
      background: linear-gradient(90deg, var(--accent), #7df8ec);
      transition: width 0.2s ease;
    }
    .muted { color: var(--muted); }
    .ok { color: var(--ok); }
    .warn-c { color: var(--warn); }
    .danger { color: var(--danger); }
    @media (max-width: 720px) {
      body { padding: 10px; }
      .title { font-size: 20px; }
      .card .v { font-size: 19px; }
      svg { height: 76px; }
      .clock-time { font-size: 19px; }
    }
  </style>
</head>
<body>
  <div class="wrap">
    <div class="top">
      <div>
        <div class="brand">
          <span class="brand-dot"></span>
          <div class="title">AURA</div>
        </div>
        <div id="deviceNameLabel" class="subtitle">Device</div>
      </div>
      <div class="top-right">
        <div class="clock">
          <div id="headerTime" class="clock-time">--:--</div>
          <div id="headerDate" class="clock-date">-- --- ----</div>
        </div>
        <div class="actions">
          <button id="refreshBtn" type="button">Refresh</button>
          <a class="link-btn" href="/">WiFi Setup</a>
        </div>
      </div>
    </div>

    <div id="status" class="status">Loading...</div>

    <div class="tabs">
      <button class="tab-btn active" type="button" data-tab="sensors">Sensors</button>
      <button class="tab-btn" type="button" data-tab="charts">Charts</button>
      <button class="tab-btn" type="button" data-tab="events">Events</button>
      <button class="tab-btn" type="button" data-tab="settings">Settings</button>
      <button class="tab-btn" type="button" data-tab="system">System</button>
    </div>

    <div id="tab-sensors" class="tab-panel active">
      <div class="sec">
        <h3>Sensors</h3>
        <div id="metrics" class="grid"></div>
      </div>
      <div class="sec">
        <h3>Derived</h3>
        <div id="derivedMeta" class="meta muted">Loading...</div>
      </div>
    </div>

    <div id="tab-charts" class="tab-panel">
      <div class="sec">
        <h3>Charts</h3>
        <div class="row">
          <div class="field">
            <label>Group</label>
            <div class="seg" id="chartGroupSeg">
              <button class="seg-btn active" type="button" data-group="core">Core</button>
              <button class="seg-btn" type="button" data-group="gases">Gases</button>
              <button class="seg-btn" type="button" data-group="pm">PM</button>
            </div>
          </div>
          <div class="field">
            <label>Range</label>
            <div class="seg" id="chartWindowSeg">
              <button class="seg-btn" type="button" data-window="1h">1h</button>
              <button class="seg-btn" type="button" data-window="3h">3h</button>
              <button class="seg-btn active" type="button" data-window="24h">24h</button>
            </div>
          </div>
        </div>
        <div id="charts" class="charts" style="margin-top:10px;"></div>
      </div>
    </div>

    <div id="tab-events" class="tab-panel">
      <div class="sec">
        <h3>Recent Events</h3>
        <div id="events" class="events muted">Loading...</div>
      </div>
    </div>

    <div id="tab-settings" class="tab-panel">
      <div class="sec">
        <h3>Device Settings</h3>
        <div class="row">
          <div class="field inline">
            <input id="setNightMode" type="checkbox" />
            <label for="setNightMode">Night mode</label>
          </div>
          <div class="field inline">
            <input id="setBacklight" type="checkbox" />
            <label for="setBacklight">Backlight ON</label>
          </div>
          <div class="field inline">
            <input id="setUnitsC" type="checkbox" />
            <label for="setUnitsC">Use Celsius</label>
          </div>
        </div>
        <div class="row" style="margin-top:10px;">
          <div class="field">
            <label for="setTempOffset">Temperature offset (C)</label>
            <input id="setTempOffset" type="number" step="0.1" />
          </div>
          <div class="field">
            <label for="setHumOffset">Humidity offset (%)</label>
            <input id="setHumOffset" type="number" step="0.1" />
          </div>
          <div class="field">
            <label for="setDisplayName">Display name</label>
            <input id="setDisplayName" type="text" maxlength="32" />
          </div>
        </div>
        <div class="btn-row">
          <button id="saveSettingsBtn" class="primary" type="button">Save</button>
          <button id="restartBtn" class="danger-btn" type="button">Save + Restart</button>
        </div>
        <p id="settingsStatus" class="note">No pending changes.</p>
      </div>
    </div>

    <div id="tab-system" class="tab-panel">
      <div class="sec">
        <h3>System</h3>
        <div id="deviceMeta" class="meta muted">Loading...</div>
        <div class="btn-row">
          <button id="rebootNowBtn" class="danger-btn" type="button">Reboot Device</button>
          <button id="openDacBtn" type="button">Open DAC Page</button>
        </div>
      </div>
      <div class="sec">
        <h3>Firmware OTA</h3>
        <p class="note">Upload `.bin` firmware file. Device will reboot after successful update.</p>
        <div class="row" style="margin-top:10px;">
          <div class="field">
            <label for="otaFile">Firmware file</label>
            <input id="otaFile" type="file" accept=".bin,application/octet-stream" />
          </div>
          <div class="field" style="align-self:end;">
            <button id="otaUploadBtn" type="button">Upload firmware</button>
          </div>
        </div>
        <div class="progress-wrap"><div id="otaProgress" class="progress"></div></div>
        <p id="otaStatus" class="note">No upload in progress.</p>
      </div>
    </div>
  </div>

  <script>
    const METRICS = [
      { key: "co2", label: "CO2", unit: "ppm", digits: 0 },
      { key: "temp", label: "Temperature", unit: "C", digits: 1 },
      { key: "rh", label: "Humidity", unit: "%", digits: 0 },
      { key: "pressure", label: "Pressure", unit: "hPa", digits: 0 },
      { key: "pm25", label: "PM2.5", unit: "ug/m3", digits: 1 },
      { key: "pm10", label: "PM10", unit: "ug/m3", digits: 1 },
      { key: "pm1", label: "PM1.0", unit: "ug/m3", digits: 1 },
      { key: "pm4", label: "PM4.0", unit: "ug/m3", digits: 1 },
      { key: "pm05", label: "PM0.5", unit: "#/cm3", digits: 0 },
      { key: "voc", label: "VOC", unit: "idx", digits: 0 },
      { key: "nox", label: "NOx", unit: "idx", digits: 0 },
      { key: "hcho", label: "HCHO", unit: "ppb", digits: 0 },
      { key: "co", label: "CO", unit: "ppm", digits: 1 }
    ];

    const CHART_META = {
      co2: { label: "CO2", unit: "ppm", digits: 0, color: "#3dd6c6" },
      temperature: { label: "Temperature", unit: "C", digits: 1, color: "#f59e0b" },
      humidity: { label: "Humidity", unit: "%", digits: 0, color: "#60a5fa" },
      pressure: { label: "Pressure", unit: "hPa", digits: 0, color: "#22c55e" },
      co: { label: "CO", unit: "ppm", digits: 1, color: "#fb7185" },
      voc: { label: "VOC", unit: "idx", digits: 0, color: "#f97316" },
      nox: { label: "NOx", unit: "idx", digits: 0, color: "#f43f5e" },
      hcho: { label: "HCHO", unit: "ppb", digits: 0, color: "#a78bfa" },
      pm05: { label: "PM0.5", unit: "#/cm3", digits: 0, color: "#fde047" },
      pm1: { label: "PM1.0", unit: "ug/m3", digits: 1, color: "#38bdf8" },
      pm25: { label: "PM2.5", unit: "ug/m3", digits: 1, color: "#34d399" },
      pm4: { label: "PM4.0", unit: "ug/m3", digits: 1, color: "#22d3ee" },
      pm10: { label: "PM10", unit: "ug/m3", digits: 1, color: "#f87171" }
    };

    function $(id) { return document.getElementById(id); }

    function isNum(value) {
      return typeof value === "number" && Number.isFinite(value);
    }

    function fmt(value, digits) {
      if (!isNum(value)) return "--";
      return value.toFixed(digits);
    }

    function esc(value) {
      const str = String(value == null ? "" : value);
      return str
        .replace(/&/g, "&amp;")
        .replace(/</g, "&lt;")
        .replace(/>/g, "&gt;")
        .replace(/"/g, "&quot;")
        .replace(/'/g, "&#39;");
    }

    function uptimeText(seconds) {
      if (!isNum(seconds)) return "--";
      const total = Math.max(0, Math.floor(seconds));
      const d = Math.floor(total / 86400);
      const h = Math.floor((total % 86400) / 3600);
      const m = Math.floor((total % 3600) / 60);
      if (d > 0) return d + "d " + h + "h " + m + "m";
      return h + "h " + m + "m";
    }

    function statusText(text, mode) {
      const el = $("status");
      el.textContent = text;
      if (mode === "warn") el.className = "status warn";
      else if (mode === "ok") el.className = "status ok";
      else el.className = "status";
    }

    async function getJson(url) {
      const response = await fetch(url, { cache: "no-store" });
      if (!response.ok) throw new Error("HTTP " + response.status + " for " + url);
      return response.json();
    }

    async function postJson(url, payload) {
      const response = await fetch(url, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(payload)
      });
      let json = null;
      try { json = await response.json(); } catch (_e) {}
      if (!response.ok) {
        const message = json && json.error ? json.error : ("HTTP " + response.status);
        throw new Error(message);
      }
      return json;
    }

    let activeTab = "sensors";
    let chartGroup = "core";
    let chartWindow = "24h";
    let stateCache = null;
    let settingsDirty = false;
    let refreshBusy = false;
    let deviceClockRef = null;
    let otaUploadInFlight = false;
    let otaRestartPending = false;

    function selectTab(tab) {
      activeTab = tab;
      document.querySelectorAll(".tab-btn").forEach((btn) => {
        btn.classList.toggle("active", btn.getAttribute("data-tab") === tab);
      });
      ["sensors", "charts", "events", "settings", "system"].forEach((id) => {
        const panel = $("tab-" + id);
        panel.classList.toggle("active", id === tab);
      });
      if (tab === "charts") {
        refreshCharts().catch(() => {});
      } else if (tab === "events") {
        refreshEvents().catch(() => {});
      }
    }

    function formatHeaderDate(date) {
      return date.toLocaleDateString("en-GB", {
        day: "2-digit",
        month: "short",
        year: "numeric"
      }).toUpperCase();
    }

    function updateHeaderClock() {
      const nowMs = Date.now();
      const now = deviceClockRef
        ? new Date(deviceClockRef.epochMs + (nowMs - deviceClockRef.capturedAtMs))
        : new Date(nowMs);
      $("headerTime").textContent = now.toLocaleTimeString([], {
        hour: "2-digit",
        minute: "2-digit",
        hour12: false
      });
      $("headerDate").textContent = formatHeaderDate(now);
    }

    function renderHeader(statePayload) {
      const network = (statePayload && statePayload.network) || {};
      const settings = (statePayload && statePayload.settings) || {};
      const display = (typeof settings.display_name === "string" && settings.display_name.trim())
        ? settings.display_name.trim()
        : (network.hostname || "aura");
      $("deviceNameLabel").textContent = display;

      if (isNum(statePayload && statePayload.time_epoch_s)) {
        deviceClockRef = {
          epochMs: statePayload.time_epoch_s * 1000,
          capturedAtMs: Date.now()
        };
      }
      updateHeaderClock();
    }

    function setSegmentActive(containerId, attrName, value) {
      const container = $(containerId);
      if (!container) return;
      container.querySelectorAll(".seg-btn").forEach((btn) => {
        btn.classList.toggle("active", btn.getAttribute(attrName) === value);
      });
    }

    function renderMetrics(statePayload) {
      const sensors = (statePayload && statePayload.sensors) || {};
      const html = METRICS.map((m) => (
        '<div class="card">' +
          '<div class="k">' + esc(m.label) + "</div>" +
          '<div class="v">' + fmt(sensors[m.key], m.digits) + '<span class="u">' + m.unit + "</span></div>" +
        "</div>"
      )).join("");
      $("metrics").innerHTML = html || '<div class="muted">No metrics</div>';
    }

    function renderDerived(statePayload) {
      const derived = (statePayload && statePayload.derived) || {};
      const entries = [
        ["Dew point", fmt(derived.dew_point, 1) + " C"],
        ["Absolute humidity", fmt(derived.ah, 1) + " g/m3"],
        ["Mold risk", isNum(derived.mold) ? String(Math.round(derived.mold)) : "--"],
        ["Pressure delta 3h", fmt(derived.pressure_delta_3h, 1) + " hPa"],
        ["Pressure delta 24h", fmt(derived.pressure_delta_24h, 1) + " hPa"],
        ["Uptime", derived.uptime || "--"]
      ];
      $("derivedMeta").innerHTML = entries.map((row) => (
        "<div><b>" + esc(row[0]) + ":</b><span>" + esc(row[1]) + "</span></div>"
      )).join("");
    }

    function renderSystemMeta(statePayload) {
      const network = (statePayload && statePayload.network) || {};
      const system = (statePayload && statePayload.system) || {};
      const derived = (statePayload && statePayload.derived) || {};
      const mode = String(network.mode || "--").toUpperCase();
      const mqttConnected = network.mqtt_connected === true;

      const entries = [
        ["Mode", mode],
        ["SSID", network.wifi_ssid || "--"],
        ["IP", network.ip || "--"],
        ["Hostname", network.hostname || "--"],
        ["RSSI", isNum(network.rssi) ? String(Math.round(network.rssi)) + " dBm" : "--"],
        ["MQTT", mqttConnected ? "Connected" : "Disconnected"],
        ["MQTT Broker", network.mqtt_broker || "--"],
        ["Firmware", system.firmware || "--"],
        ["Build", [system.build_date, system.build_time].filter(Boolean).join(" ") || "--"],
        ["Uptime", system.uptime || derived.uptime || "--"],
        ["DAC available", system.dac_available ? "Yes" : "No"]
      ];

      $("deviceMeta").innerHTML = entries.map((row) => {
        const key = row[0];
        const value = String(row[1]);
        const cls = key === "MQTT" ? (mqttConnected ? "ok" : "danger") : "";
        return '<div><b>' + esc(key) + ':</b><span class="' + cls + '">' + esc(value) + "</span></div>";
      }).join("");

      const dacBtn = $("openDacBtn");
      if (dacBtn) {
        dacBtn.disabled = system.dac_available !== true;
      }
    }

    function chartLiveValue(key) {
      const sensors = stateCache && stateCache.sensors ? stateCache.sensors : {};
      if (key === "temperature") return sensors.temp;
      if (key === "humidity") return sensors.rh;
      return sensors[key];
    }

    function polylineFromValues(values, width, height, pad) {
      const finite = values.filter((v) => isNum(v));
      if (!finite.length) return "";
      let min = Math.min.apply(null, finite);
      let max = Math.max.apply(null, finite);
      if (max <= min) max = min + 1;
      const span = max - min;
      const last = values.length - 1;
      const pts = [];
      for (let i = 0; i < values.length; i++) {
        const v = values[i];
        if (!isNum(v)) continue;
        const x = pad + (last > 0 ? (i / last) * (width - pad * 2) : width / 2);
        const y = height - pad - ((v - min) / span) * (height - pad * 2);
        pts.push(x.toFixed(1) + "," + y.toFixed(1));
      }
      return pts.join(" ");
    }

    function renderCharts(chartsPayload) {
      const timestamps = Array.isArray(chartsPayload && chartsPayload.timestamps) ? chartsPayload.timestamps : [];
      const series = Array.isArray(chartsPayload && chartsPayload.series) ? chartsPayload.series : [];
      if (!series.length) {
        $("charts").innerHTML = '<div class="muted">No chart data</div>';
        return;
      }

      const width = 360;
      const height = 86;
      const pad = 7;
      const lastTs = timestamps.length ? timestamps[timestamps.length - 1] : null;
      const lastTime = isNum(lastTs)
        ? new Date(lastTs * 1000).toLocaleTimeString([], { hour: "2-digit", minute: "2-digit", hour12: false })
        : "--:--";

      $("charts").innerHTML = series.map((s) => {
        if (!s || typeof s.key !== "string") return "";
        const meta = CHART_META[s.key] || {
          label: s.key.toUpperCase(),
          unit: s.unit || "",
          digits: 1,
          color: "#3dd6c6"
        };
        const values = Array.isArray(s.values) ? s.values : [];
        const live = chartLiveValue(s.key);
        const latest = isNum(live) ? live : (isNum(s.latest) ? s.latest : null);
        const points = polylineFromValues(values, width, height, pad);
        const valText = fmt(latest, meta.digits) + (meta.unit ? " " + meta.unit : "");
        return (
          '<div class="chart-box">' +
            '<div class="chart-head">' +
              '<div class="chart-name">' + esc(meta.label) + "</div>" +
              '<div class="chart-latest">' + esc(valText) + " @ " + esc(lastTime) + "</div>" +
            "</div>" +
            '<svg viewBox="0 0 ' + width + " " + height + '" preserveAspectRatio="none">' +
              '<polyline fill="none" stroke="#1e2b4f" stroke-width="1.0" points="0,' + (height / 2) + " " + width + "," + (height / 2) + '"></polyline>' +
              '<polyline fill="none" stroke="' + meta.color + '" stroke-width="2.2" points="' + points + '"></polyline>' +
            "</svg>" +
          "</div>"
        );
      }).join("");
    }

    function renderEvents(eventsPayload) {
      const events = Array.isArray(eventsPayload && eventsPayload.events) ? eventsPayload.events : [];
      if (!events.length) {
        $("events").innerHTML = '<div class="muted">No events</div>';
        return;
      }
      $("events").innerHTML = events.slice(0, 24).map((e) => {
        const sev = String(e && e.severity ? e.severity : "info");
        const levelClass = (sev === "critical" || sev === "danger") ? "critical" : (sev === "warning" ? "warning" : "");
        const type = e && e.type ? e.type : "SYSTEM";
        const msg = e && e.message ? e.message : "Event";
        const ts = isNum(e && e.ts_ms) ? ("t+" + uptimeText(e.ts_ms / 1000)) : "--";
        return (
          '<div class="ev ' + levelClass + '">' +
            '<div class="ev-top"><span>' + esc(type) + "</span><span>" + esc(sev) + " / " + esc(ts) + "</span></div>" +
            '<div class="ev-msg">' + esc(msg) + "</div>" +
          "</div>"
        );
      }).join("");
    }

    function applySettingsForm(statePayload, force) {
      const settings = (statePayload && statePayload.settings) || {};
      if (settingsDirty && !force) return;

      if (typeof settings.night_mode === "boolean") $("setNightMode").checked = settings.night_mode;
      if (typeof settings.backlight_on === "boolean") $("setBacklight").checked = settings.backlight_on;
      if (typeof settings.units_c === "boolean") $("setUnitsC").checked = settings.units_c;
      $("setNightMode").disabled = settings.night_mode_locked === true;

      if (isNum(settings.temp_offset)) $("setTempOffset").value = settings.temp_offset.toFixed(1);
      else $("setTempOffset").value = "";

      if (isNum(settings.hum_offset)) $("setHumOffset").value = settings.hum_offset.toFixed(1);
      else $("setHumOffset").value = "";

      $("setDisplayName").value = typeof settings.display_name === "string" ? settings.display_name : "";
      settingsDirty = false;
      const lockHint = settings.night_mode_locked ? " Night mode is locked by auto mode." : "";
      $("settingsStatus").textContent = "Settings loaded." + lockHint;
      $("settingsStatus").className = "note muted";
    }

    function readNumInput(id, fallback) {
      const raw = $(id).value.trim();
      if (raw === "") return fallback;
      const num = Number(raw);
      if (!Number.isFinite(num)) return fallback;
      return num;
    }

    function collectSettingsPayload(restart) {
      const current = (stateCache && stateCache.settings) || {};
      const payload = {
        backlight_on: $("setBacklight").checked,
        units_c: $("setUnitsC").checked,
        temp_offset: readNumInput("setTempOffset", isNum(current.temp_offset) ? current.temp_offset : 0),
        hum_offset: readNumInput("setHumOffset", isNum(current.hum_offset) ? current.hum_offset : 0),
        display_name: $("setDisplayName").value.trim()
      };
      if (!$("setNightMode").disabled) {
        payload.night_mode = $("setNightMode").checked;
      }
      if (restart) payload.restart = true;
      return payload;
    }

    async function saveSettings(restart) {
      const status = $("settingsStatus");
      status.textContent = restart ? "Saving settings and requesting restart..." : "Saving settings...";
      status.className = "note muted";
      try {
        const payload = collectSettingsPayload(restart);
        const result = await postJson("/api/settings", payload);
        settingsDirty = false;
        if (result && result.settings) {
          stateCache = stateCache || {};
          stateCache.settings = result.settings;
          applySettingsForm({ settings: result.settings }, true);
        }
        status.textContent = restart
          ? "Settings saved. Restart requested."
          : "Settings saved.";
        status.className = "note ok";
        await refreshState();
      } catch (err) {
        status.textContent = "Save failed: " + (err && err.message ? err.message : "unknown");
        status.className = "note danger";
      }
    }

    async function uploadOta() {
      const fileInput = $("otaFile");
      const statusEl = $("otaStatus");
      const progressEl = $("otaProgress");
      const uploadBtn = $("otaUploadBtn");
      const file = fileInput.files && fileInput.files[0];
      if (otaUploadInFlight || otaRestartPending) {
        return;
      }
      if (!file) {
        statusEl.textContent = "Select firmware file first.";
        statusEl.className = "note warn-c";
        return;
      }

      otaUploadInFlight = true;
      if (uploadBtn) {
        uploadBtn.disabled = true;
      }
      statusEl.textContent = "Uploading firmware...";
      statusEl.className = "note muted";
      progressEl.style.width = "0%";

      const xhr = new XMLHttpRequest();
      const form = new FormData();
      form.append("ota_size", String(file.size));
      form.append("firmware", file, file.name);

      xhr.open("POST", "/api/ota", true);
      xhr.upload.onprogress = (event) => {
        if (!event.lengthComputable || event.total <= 0) return;
        const progress = Math.min(100, Math.round((event.loaded / event.total) * 100));
        progressEl.style.width = progress + "%";
      };

      xhr.onreadystatechange = () => {
        if (xhr.readyState !== XMLHttpRequest.DONE) return;
        otaUploadInFlight = false;
        let payload = null;
        try { payload = JSON.parse(xhr.responseText || "{}"); } catch (_e) {}

        if (xhr.status >= 200 && xhr.status < 300 && payload && payload.success === true) {
          otaRestartPending = true;
          if (uploadBtn) {
            uploadBtn.disabled = true;
          }
          progressEl.style.width = "100%";
          statusEl.textContent = payload.message || "Firmware uploaded. Device will reboot.";
          statusEl.className = "note ok";
          fileInput.value = "";
          return;
        }

        otaRestartPending = false;
        if (uploadBtn) {
          uploadBtn.disabled = false;
        }
        const errorText = (payload && payload.error) || ("Upload failed (HTTP " + (xhr.status || 0) + ")");
        statusEl.textContent = errorText;
        statusEl.className = "note danger";
      };

      xhr.onerror = () => {
        otaUploadInFlight = false;
        otaRestartPending = false;
        if (uploadBtn) {
          uploadBtn.disabled = false;
        }
        statusEl.textContent = "Upload failed. Check AP connection and retry.";
        statusEl.className = "note danger";
      };

      xhr.send(form);
    }

    async function refreshState() {
      if (otaUploadInFlight || otaRestartPending) return;
      const payload = await getJson("/api/state");
      stateCache = payload;
      renderHeader(payload);
      renderMetrics(payload);
      renderDerived(payload);
      renderSystemMeta(payload);
      applySettingsForm(payload, false);
    }

    async function refreshCharts() {
      if (otaUploadInFlight || otaRestartPending) return;
      const payload = await getJson(
        "/api/charts?group=" + encodeURIComponent(chartGroup) +
        "&window=" + encodeURIComponent(chartWindow)
      );
      renderCharts(payload);
    }

    async function refreshEvents() {
      if (otaUploadInFlight || otaRestartPending) return;
      const payload = await getJson("/api/events");
      renderEvents(payload);
    }

    async function refreshActive() {
      if (refreshBusy || otaUploadInFlight || otaRestartPending) return;
      refreshBusy = true;
      let hadError = false;

      try {
        await refreshState();
      } catch (err) {
        hadError = true;
        statusText("State API error: " + (err && err.message ? err.message : "unknown"), "warn");
      }

      if (activeTab === "charts") {
        try { await refreshCharts(); }
        catch (err) {
          hadError = true;
          statusText("Charts API error: " + (err && err.message ? err.message : "unknown"), "warn");
        }
      }
      if (activeTab === "events") {
        try { await refreshEvents(); }
        catch (err) {
          hadError = true;
          statusText("Events API error: " + (err && err.message ? err.message : "unknown"), "warn");
        }
      }

      if (!hadError) {
        statusText(
          "AP offline dashboard active. Last update: " +
            new Date().toLocaleTimeString([], { hour: "2-digit", minute: "2-digit", second: "2-digit", hour12: false }),
          "ok"
        );
      }
      refreshBusy = false;
    }

    function bindUi() {
      document.querySelectorAll(".tab-btn").forEach((btn) => {
        btn.addEventListener("click", () => selectTab(btn.getAttribute("data-tab")));
      });

      $("refreshBtn").addEventListener("click", async () => {
        if (otaUploadInFlight || otaRestartPending) {
          statusText("OTA upload in progress. Wait for completion.", "warn");
          return;
        }
        try {
          await refreshState();
          await refreshCharts();
          await refreshEvents();
          statusText(
            "AP offline dashboard active. Last update: " +
              new Date().toLocaleTimeString([], { hour: "2-digit", minute: "2-digit", second: "2-digit", hour12: false }),
            "ok"
          );
        } catch (err) {
          statusText("Refresh failed: " + (err && err.message ? err.message : "unknown"), "warn");
        }
      });

      document.querySelectorAll("#chartGroupSeg .seg-btn").forEach((btn) => {
        btn.addEventListener("click", () => {
          chartGroup = btn.getAttribute("data-group") || "core";
          setSegmentActive("chartGroupSeg", "data-group", chartGroup);
          refreshCharts().catch(() => {});
        });
      });
      document.querySelectorAll("#chartWindowSeg .seg-btn").forEach((btn) => {
        btn.addEventListener("click", () => {
          chartWindow = btn.getAttribute("data-window") || "24h";
          setSegmentActive("chartWindowSeg", "data-window", chartWindow);
          refreshCharts().catch(() => {});
        });
      });

      ["setNightMode", "setBacklight", "setUnitsC", "setTempOffset", "setHumOffset", "setDisplayName"].forEach((id) => {
        $(id).addEventListener("input", () => {
          settingsDirty = true;
          $("settingsStatus").textContent = "Unsaved changes.";
          $("settingsStatus").className = "note warn-c";
        });
        $(id).addEventListener("change", () => {
          settingsDirty = true;
          $("settingsStatus").textContent = "Unsaved changes.";
          $("settingsStatus").className = "note warn-c";
        });
      });

      $("saveSettingsBtn").addEventListener("click", () => saveSettings(false));
      $("restartBtn").addEventListener("click", () => saveSettings(true));
      $("otaUploadBtn").addEventListener("click", uploadOta);
      $("rebootNowBtn").addEventListener("click", async () => {
        try {
          await postJson("/api/settings", { restart: true });
          statusText("Restart requested.", "ok");
        } catch (err) {
          statusText("Restart request failed: " + (err && err.message ? err.message : "unknown"), "warn");
        }
      });
      $("openDacBtn").addEventListener("click", () => {
        window.location.href = "/dac";
      });
    }

    bindUi();
    updateHeaderClock();
    setSegmentActive("chartGroupSeg", "data-group", chartGroup);
    setSegmentActive("chartWindowSeg", "data-window", chartWindow);
    refreshState().then(() => applySettingsForm(stateCache, true)).catch(() => {});
    refreshCharts().catch(() => {});
    refreshEvents().catch(() => {});
    setInterval(refreshActive, 10000);
    setInterval(updateHeaderClock, 1000);
  </script>
</body>
</html>
)HTML_DASH_AP";

} // namespace WebTemplates
