#pragma once

#include <Arduino.h>

static const char kWebDashboardHtml[] PROGMEM = R"HTML(
<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>Drone FC Telemetry</title>
  <style>
    :root {
      --bg: #071112;
      --panel: rgba(10, 20, 22, 0.9);
      --line: rgba(127, 218, 178, 0.16);
      --accent: #7fdab2;
      --warn: #f2bf6d;
      --good: #8ce08f;
      --bad: #ff7c71;
      --text: #edf5f1;
      --muted: #96aba5;
      --mono: "Consolas", "SFMono-Regular", monospace;
      --ui: "Trebuchet MS", "Segoe UI", sans-serif;
    }

    * { box-sizing: border-box; }
    body {
      margin: 0;
      min-height: 100vh;
      font-family: var(--ui);
      color: var(--text);
      background:
        radial-gradient(circle at top, rgba(68, 126, 111, 0.35), transparent 36%),
        linear-gradient(180deg, #0d1718, #050809);
    }

    header {
      padding: 22px 20px 8px;
      display: flex;
      flex-wrap: wrap;
      gap: 12px;
      justify-content: space-between;
      align-items: center;
    }

    h1 { margin: 0; font-size: 1.6rem; }
    .sub { margin-top: 6px; color: var(--muted); font-size: 0.92rem; }

    .chips {
      display: flex;
      flex-wrap: wrap;
      gap: 8px;
      align-items: center;
    }

    .page-nav {
      padding: 0 20px 16px;
      display: flex;
      flex-wrap: wrap;
      gap: 10px;
    }

    .page-btn {
      border: 1px solid var(--line);
      border-radius: 999px;
      padding: 10px 14px;
      background: rgba(255, 255, 255, 0.04);
      color: var(--muted);
      font-family: var(--mono);
      font-size: 0.8rem;
      letter-spacing: 0.08em;
      text-transform: uppercase;
      cursor: pointer;
    }

    .page-btn.active {
      color: #081010;
      background: linear-gradient(90deg, var(--warn), var(--accent));
      border-color: transparent;
    }

    .chip {
      padding: 8px 12px;
      border-radius: 999px;
      border: 1px solid var(--line);
      background: rgba(255, 255, 255, 0.05);
      color: var(--muted);
      font-size: 0.82rem;
    }

    .chip.good {
      border-color: rgba(140, 224, 143, 0.45);
      color: var(--good);
    }

    .chip.warn {
      border-color: rgba(242, 191, 109, 0.45);
      color: var(--warn);
    }

    .chip.bad {
      border-color: rgba(255, 124, 113, 0.45);
      color: var(--bad);
    }

    .lang-wrap {
      display: flex;
      align-items: center;
      gap: 8px;
      padding: 8px 12px;
      border-radius: 999px;
      border: 1px solid var(--line);
      background: rgba(255, 255, 255, 0.05);
      color: var(--muted);
      font-size: 0.78rem;
    }

    .lang-select {
      border: 1px solid rgba(127, 218, 178, 0.18);
      border-radius: 999px;
      background: rgba(5, 10, 11, 0.92);
      color: var(--text);
      padding: 6px 10px;
      font-family: var(--mono);
      outline: none;
    }

    .dash {
      padding: 0 20px 24px;
      display: grid;
      gap: 16px;
      grid-template-columns: repeat(auto-fit, minmax(310px, 1fr));
    }

    .card {
      background: var(--panel);
      border: 1px solid var(--line);
      border-radius: 24px;
      padding: 18px;
      box-shadow: 0 24px 54px rgba(0, 0, 0, 0.22);
    }

    .page-section[hidden] {
      display: none !important;
    }

    .span2 { grid-column: span 2; }

    .card h2 {
      margin: 0 0 14px;
      font-size: 0.9rem;
      letter-spacing: 0.14em;
      text-transform: uppercase;
      color: var(--accent);
    }

    .grid {
      display: grid;
      gap: 10px;
      grid-template-columns: repeat(2, minmax(0, 1fr));
    }

    .meta {
      padding: 11px 12px;
      border-radius: 16px;
      background: rgba(255, 255, 255, 0.03);
      border: 1px solid var(--line);
    }

    .label {
      color: var(--muted);
      font-size: 0.72rem;
      text-transform: uppercase;
      letter-spacing: 0.08em;
    }

    .value {
      margin-top: 6px;
      font-size: 1.02rem;
      font-family: var(--mono);
      word-break: break-word;
    }

    .stage {
      position: relative;
      margin-top: 12px;
      height: 290px;
      display: flex;
      align-items: center;
      justify-content: center;
      perspective: 900px;
      overflow: hidden;
      background:
        radial-gradient(circle at center, rgba(127, 218, 178, 0.14), transparent 42%),
        linear-gradient(180deg, rgba(7, 14, 15, 0.96), rgba(7, 12, 13, 0.98));
      border: 1px solid var(--line);
      border-radius: 22px;
    }

    .stage-badge {
      position: absolute;
      left: 16px;
      padding: 7px 10px;
      border-radius: 999px;
      border: 1px solid var(--line);
      background: rgba(7, 17, 18, 0.8);
      color: var(--muted);
      font-size: 0.72rem;
      font-family: var(--mono);
      letter-spacing: 0.12em;
      text-transform: uppercase;
    }

    .stage-badge.front {
      top: 16px;
      color: var(--warn);
      border-color: rgba(242, 191, 109, 0.35);
    }

    .stage-badge.rear {
      bottom: 16px;
      color: var(--accent);
    }

    .stage-guide {
      position: absolute;
      right: 16px;
      top: 16px;
      max-width: 150px;
      padding: 10px 12px;
      border-radius: 16px;
      border: 1px solid var(--line);
      background: rgba(7, 17, 18, 0.76);
      color: var(--muted);
      font-size: 0.76rem;
      line-height: 1.45;
    }

    .drone-shadow {
      position: absolute;
      width: 220px;
      height: 220px;
      border-radius: 50%;
      background: radial-gradient(circle, rgba(127, 218, 178, 0.14), transparent 65%);
      filter: blur(12px);
      transform: translateY(28px);
    }

    .drone {
      position: relative;
      width: 188px;
      height: 188px;
      transform-style: preserve-3d;
      transition: transform 0.18s ease-out;
    }

    .arm {
      position: absolute;
      inset: 0;
      margin: auto;
      width: 138px;
      height: 12px;
      border-radius: 999px;
      background: linear-gradient(90deg, var(--warn), var(--accent));
      box-shadow: 0 0 24px rgba(127, 218, 178, 0.16);
    }

    .a { transform: rotate(45deg) translateZ(18px); }
    .b { transform: rotate(-45deg) translateZ(18px); }

    .nose {
      position: absolute;
      top: 6px;
      left: 50%;
      width: 0;
      height: 0;
      border-left: 12px solid transparent;
      border-right: 12px solid transparent;
      border-bottom: 24px solid var(--bad);
      transform: translateX(-50%) translateZ(40px);
      filter: drop-shadow(0 0 14px rgba(255, 124, 113, 0.45));
    }

    .tail {
      position: absolute;
      left: 50%;
      bottom: 18px;
      width: 44px;
      height: 10px;
      border-radius: 999px;
      background: linear-gradient(90deg, rgba(127, 218, 178, 0.12), rgba(127, 218, 178, 0.75), rgba(127, 218, 178, 0.12));
      transform: translateX(-50%) translateZ(22px);
    }

    .hub {
      position: absolute;
      inset: 0;
      margin: auto;
      width: 68px;
      height: 68px;
      border-radius: 22px;
      background: linear-gradient(180deg, #173231, #0d1717);
      transform: translateZ(28px) rotate(12deg);
      box-shadow: inset 0 0 0 1px rgba(127, 218, 178, 0.12), 0 12px 28px rgba(0, 0, 0, 0.28);
    }

    .hub::after {
      content: "FC";
      position: absolute;
      left: 50%;
      top: 50%;
      transform: translate(-50%, -50%);
      color: var(--accent);
      font-size: 0.86rem;
      font-family: var(--mono);
      letter-spacing: 0.1em;
    }

    .mot {
      position: absolute;
      width: 34px;
      height: 34px;
      border-radius: 50%;
      background: radial-gradient(circle at 30% 30%, #3e6a5f, #13201f 70%);
      border: 1px solid rgba(127, 218, 178, 0.14);
      transform: translateZ(35px);
      box-shadow: 0 0 22px rgba(127, 218, 178, 0.12);
    }

    .mot::after {
      position: absolute;
      left: 50%;
      top: 50%;
      transform: translate(-50%, -50%);
      color: var(--text);
      font-size: 0.68rem;
      font-family: var(--mono);
      letter-spacing: 0.08em;
    }

    .m1 { left: 18px; top: 18px; }
    .m1::after { content: "M1"; }
    .m2 { right: 18px; top: 18px; }
    .m2::after { content: "M2"; }
    .m3 { left: 18px; bottom: 18px; }
    .m3::after { content: "M3"; }
    .m4 { right: 18px; bottom: 18px; }
    .m4::after { content: "M4"; }

    .field-grid {
      display: grid;
      gap: 10px;
      grid-template-columns: repeat(2, minmax(0, 1fr));
    }

    .pid-grid {
      display: grid;
      gap: 12px;
      grid-template-columns: repeat(auto-fit, minmax(210px, 1fr));
    }

    .mini-grid {
      display: grid;
      gap: 8px;
      grid-template-columns: repeat(3, minmax(0, 1fr));
    }

    .field {
      display: grid;
      gap: 8px;
      padding: 11px 12px;
      border-radius: 16px;
      background: rgba(255, 255, 255, 0.03);
      border: 1px solid var(--line);
    }

    .field span {
      color: var(--muted);
      font-size: 0.72rem;
      text-transform: uppercase;
      letter-spacing: 0.08em;
    }

    .input {
      width: 100%;
      border: 1px solid rgba(127, 218, 178, 0.14);
      border-radius: 12px;
      padding: 10px 11px;
      color: var(--text);
      background: rgba(5, 10, 11, 0.9);
      font-family: var(--mono);
      outline: none;
    }

    .input:focus {
      border-color: rgba(127, 218, 178, 0.42);
      box-shadow: 0 0 0 3px rgba(127, 218, 178, 0.1);
    }

    .input:disabled {
      opacity: 0.55;
      cursor: not-allowed;
    }

    .checkline {
      display: flex;
      align-items: center;
      gap: 10px;
      color: var(--text);
      font-size: 0.92rem;
      font-family: var(--mono);
    }

    .checkline input {
      width: 18px;
      height: 18px;
      accent-color: var(--accent);
    }

    .rotation-board {
      position: relative;
      margin-top: 12px;
      height: 300px;
      overflow: hidden;
      border-radius: 22px;
      border: 1px solid var(--line);
      background:
        radial-gradient(circle at 50% 32%, rgba(127, 218, 178, 0.12), transparent 46%),
        linear-gradient(180deg, rgba(12, 20, 20, 0.96), rgba(6, 10, 11, 0.98));
    }

    .rotation-front {
      position: absolute;
      top: 14px;
      left: 50%;
      transform: translateX(-50%);
      padding: 7px 12px;
      border-radius: 999px;
      background: rgba(242, 191, 109, 0.1);
      border: 1px solid rgba(242, 191, 109, 0.25);
      color: var(--warn);
      font-size: 0.74rem;
      font-family: var(--mono);
      letter-spacing: 0.12em;
    }

    .rotation-arm {
      position: absolute;
      left: 50%;
      top: 50%;
      width: 180px;
      height: 8px;
      border-radius: 999px;
      background: linear-gradient(90deg, rgba(242, 191, 109, 0.82), rgba(127, 218, 178, 0.82));
      transform-origin: center;
      opacity: 0.82;
    }

    .rotation-arm.a { transform: translate(-50%, -50%) rotate(45deg); }
    .rotation-arm.b { transform: translate(-50%, -50%) rotate(-45deg); }

    .rotation-core {
      position: absolute;
      left: 50%;
      top: 50%;
      width: 72px;
      height: 72px;
      border-radius: 22px;
      background: linear-gradient(180deg, #173231, #0d1717);
      transform: translate(-50%, -50%) rotate(12deg);
      box-shadow: inset 0 0 0 1px rgba(127, 218, 178, 0.12);
      color: var(--accent);
      display: flex;
      align-items: center;
      justify-content: center;
      font-family: var(--mono);
      letter-spacing: 0.12em;
    }

    .rotor {
      position: absolute;
      width: 82px;
      height: 82px;
      border-radius: 50%;
      border: 1px solid rgba(127, 218, 178, 0.18);
      background: rgba(10, 17, 18, 0.88);
      display: flex;
      align-items: center;
      justify-content: center;
      flex-direction: column;
      gap: 3px;
      box-shadow: 0 12px 26px rgba(0, 0, 0, 0.18);
    }

    .rotor-ring {
      position: absolute;
      inset: 10px;
      border-radius: 50%;
      border: 2px dashed rgba(127, 218, 178, 0.45);
    }

    .rotor.cw .rotor-ring { animation: spin-cw 4s linear infinite; }
    .rotor.ccw .rotor-ring { animation: spin-ccw 4s linear infinite; }

    .rotor-name {
      position: relative;
      z-index: 1;
      font-family: var(--mono);
      font-size: 0.9rem;
      color: var(--text);
    }

    .rotor-dir {
      position: relative;
      z-index: 1;
      font-size: 0.7rem;
      letter-spacing: 0.08em;
      color: var(--muted);
    }

    .rotor.m1 { left: 54px; top: 64px; }
    .rotor.m2 { right: 54px; top: 64px; }
    .rotor.m3 { left: 54px; bottom: 36px; }
    .rotor.m4 { right: 54px; bottom: 36px; }

    @keyframes spin-cw {
      from { transform: rotate(0deg); }
      to { transform: rotate(360deg); }
    }

    @keyframes spin-ccw {
      from { transform: rotate(0deg); }
      to { transform: rotate(-360deg); }
    }

    @media (max-width: 720px) {
      .field-grid {
        grid-template-columns: 1fr;
      }

      .stage-guide {
        right: 12px;
        top: auto;
        bottom: 12px;
        max-width: 140px;
      }
    }
    canvas {
      width: 100%;
      height: 260px;
      display: block;
      border-radius: 20px;
      border: 1px solid var(--line);
      background: linear-gradient(180deg, rgba(16, 28, 30, 0.98), rgba(7, 12, 13, 0.98));
    }

    .list {
      display: grid;
      gap: 10px;
    }

    .row {
      display: grid;
      grid-template-columns: 60px 1fr 68px;
      gap: 10px;
      align-items: center;
    }

    .lab {
      color: var(--muted);
      font-size: 0.8rem;
      text-transform: uppercase;
    }

    .shell {
      height: 12px;
      border-radius: 999px;
      background: rgba(255, 255, 255, 0.08);
      border: 1px solid rgba(255, 255, 255, 0.05);
      overflow: hidden;
    }

    .fill {
      height: 100%;
      width: 0;
      background: linear-gradient(90deg, var(--warn), var(--accent));
      transition: width 0.15s ease;
    }

    .row.stale .fill {
      background: linear-gradient(90deg, #556766, #83908d);
    }

    .num {
      text-align: right;
      font-size: 0.8rem;
      font-family: var(--mono);
      color: var(--text);
    }

    .stack {
      display: grid;
      gap: 8px;
    }

    .kv {
      display: grid;
      gap: 8px;
    }

    .kv div {
      padding: 9px 11px;
      border-radius: 14px;
      border: 1px solid var(--line);
      background: rgba(255, 255, 255, 0.03);
      font-size: 0.88rem;
      line-height: 1.5;
    }

    .kv strong {
      color: var(--accent);
      margin-right: 8px;
      font-family: var(--mono);
    }

    .tag {
      padding: 6px 10px;
      border-radius: 999px;
      border: 1px solid var(--line);
      background: rgba(255, 255, 255, 0.05);
      color: var(--muted);
      font-size: 0.76rem;
      font-family: var(--mono);
    }

    .note {
      margin: 10px 0 0;
      color: var(--muted);
      font-size: 0.86rem;
      line-height: 1.5;
    }

    .actions {
      margin-top: 12px;
      display: grid;
      gap: 10px;
      grid-template-columns: repeat(auto-fit, minmax(130px, 1fr));
    }

    button {
      border: 0;
      border-radius: 14px;
      padding: 11px 13px;
      font-weight: 700;
      cursor: pointer;
      color: #081010;
      background: linear-gradient(90deg, var(--warn), var(--accent));
    }

    button.alt {
      background: rgba(255, 255, 255, 0.08);
      color: var(--text);
      border: 1px solid var(--line);
    }

    button:disabled {
      cursor: not-allowed;
      opacity: 0.45;
    }

    button.page-btn {
      border: 1px solid var(--line);
      border-radius: 999px;
      padding: 10px 14px;
      background: rgba(255, 255, 255, 0.04);
      color: var(--muted);
      font-family: var(--mono);
      font-size: 0.8rem;
      letter-spacing: 0.08em;
      text-transform: uppercase;
    }

    button.page-btn.active {
      color: #081010;
      background: linear-gradient(90deg, var(--warn), var(--accent));
      border-color: transparent;
    }

    .toast {
      position: fixed;
      right: 18px;
      bottom: 18px;
      padding: 12px 14px;
      border-radius: 16px;
      border: 1px solid var(--line);
      background: rgba(12, 20, 20, 0.96);
      opacity: 0;
      transform: translateY(8px);
      transition: opacity 0.18s, transform 0.18s;
      pointer-events: none;
    }

    .toast.show {
      opacity: 1;
      transform: translateY(0);
    }

    .toast.bad {
      border-color: rgba(255, 124, 113, 0.45);
    }

    @media (max-width: 980px) {
      .span2 { grid-column: auto; }
    }
  </style>
</head>
<body>
  <header>
    <div>
      <h1 id="titleText">Drone Control Surface</h1>
      <div class="sub" id="subtitleText">Receiver map: CH1 Roll, CH2 Pitch, CH3 Throttle, CH4 Yaw. Network mode now supports AP + optional STA backup.</div>
    </div>
    <div class="chips">
      <div class="chip" id="modeChip">MODE --</div>
      <div class="chip" id="armChip">ARM --</div>
      <div class="chip" id="loopChip">LOOP --</div>
      <div class="chip" id="wifiChip">IP --</div>
      <div class="chip" id="motorChip">PWM --</div>
      <div class="chip" id="linkChip">LINK --</div>
      <label class="lang-wrap" for="langSelect">
        <span id="langLabel">LANG</span>
        <select class="lang-select" id="langSelect">
          <option value="ar">العربية</option>
          <option value="zh">中文</option>
          <option value="en">English</option>
        </select>
      </label>
    </div>
  </header>

  <nav class="page-nav" id="pageNav">
    <button class="page-btn active" id="pageBtnInfo" data-target="info">Info</button>
    <button class="page-btn" id="pageBtnPid" data-target="pid">PID</button>
    <button class="page-btn" id="pageBtnVisual" data-target="visual">3D</button>
    <button class="page-btn" id="pageBtnCalibration" data-target="calibration">Cal</button>
    <button class="page-btn" id="pageBtnReceiver" data-target="receiver">RX</button>
    <button class="page-btn" id="pageBtnMotors" data-target="motors">Motors</button>
  </nav>

  <main class="dash">
    <section class="card span2 page-section" data-page="info">
      <h2 id="overviewTitle">Drone Information</h2>
      <div class="grid">
        <div class="meta"><div class="label">Battery Voltage</div><div class="value" id="batteryValue">--</div></div>
        <div class="meta"><div class="label">CPU Usage</div><div class="value" id="cpuValue">0 %</div></div>
        <div class="meta"><div class="label">Wi-Fi Signal</div><div class="value" id="rssiValue">--</div></div>
        <div class="meta"><div class="label">Flight Mode</div><div class="value" id="flightModeValue">--</div></div>
        <div class="meta"><div class="label">System Status</div><div class="value" id="systemStatusValue">--</div></div>
        <div class="meta"><div class="label">Uptime</div><div class="value" id="uptimeValue">--</div></div>
        <div class="meta"><div class="label">GPS Fix</div><div class="value" id="gpsFixValue">--</div></div>
        <div class="meta"><div class="label">Web Link</div><div class="value" id="linkModeValue">--</div></div>
      </div>
      <p class="note" id="overviewNote">Battery voltage stays unavailable until an ADC pin is assigned in firmware. CPU usage is the estimated control-loop load.</p>
    </section>

    <section class="card span2 page-section" data-page="visual">
      <h2 id="setupTitle">3D Drone Visualization</h2>
      <div class="grid">
        <div class="meta"><div class="label">Roll</div><div class="value" id="rollValue">0.0 deg</div></div>
        <div class="meta"><div class="label">Pitch</div><div class="value" id="pitchValue">0.0 deg</div></div>
        <div class="meta"><div class="label">Yaw</div><div class="value" id="yawValue">0.0 deg</div></div>
        <div class="meta"><div class="label">Arming Status</div><div class="value" id="armingReasonValue">--</div></div>
      </div>
      <div class="stage">
        <div class="stage-badge front" id="stageFrontBadge">FRONT</div>
        <div class="stage-badge rear" id="stageRearBadge">REAR</div>
        <div class="stage-guide" id="stageGuideText">Bright nose marker = front side. Tail marker = rear side.</div>
        <div class="drone-shadow"></div>
        <div class="drone" id="droneModel">
          <div class="nose"></div>
          <div class="tail"></div>
          <div class="arm a"></div>
          <div class="arm b"></div>
          <div class="hub"></div>
          <div class="mot m1"></div>
          <div class="mot m2"></div>
          <div class="mot m3"></div>
          <div class="mot m4"></div>
        </div>
      </div>
      <p class="note" id="currentMapNote">Current map: CH1 Roll, CH2 Pitch, CH3 Throttle, CH4 Yaw. Motor map: M1 front-left, M2 front-right, M3 rear-left, M4 rear-right.</p>
      <p class="note" id="linkNote">CH8 controls website behavior: 1000 normal send/receive, 1500 telemetry-only, 2000 receive-priority.</p>
    </section>

    <section class="card span2 page-section" data-page="info">
      <h2 id="gpsTitle">GPS</h2>
      <canvas id="gpsCanvas" width="720" height="360"></canvas>
      <div class="grid" style="margin-top:12px">
        <div class="meta"><div class="label">Satellites</div><div class="value" id="gpsSat">0</div></div>
        <div class="meta"><div class="label">HDOP</div><div class="value" id="gpsHdop">--</div></div>
        <div class="meta"><div class="label">Speed</div><div class="value" id="gpsSpeed">0.0 m/s</div></div>
        <div class="meta"><div class="label">Home Distance</div><div class="value" id="gpsHome">0.0 m</div></div>
        <div class="meta"><div class="label">Coordinates</div><div class="value" id="gpsCoord">--</div></div>
        <div class="meta"><div class="label">Home State</div><div class="value" id="gpsHomeState">CLEAR</div></div>
      </div>
      <div class="actions">
        <button class="alt" id="setHomeBtn">Set Home</button>
        <button class="alt" id="clearHomeBtn">Clear Home</button>
      </div>
      <p class="note" id="mapNote">The GPS panel includes an online map when the browser still has internet access. If tiles are unreachable, it falls back to the local radar view.</p>
    </section>

    <section class="card span2 page-section" data-page="pid">
      <h2 id="pidEditorTitle">PID Tuning</h2>
      <div class="pid-grid">
        <label class="field">
          <span id="rollPidLabel">Roll PID</span>
          <div class="mini-grid">
            <input class="input" id="rollKpInput" inputmode="decimal" placeholder="Kp">
            <input class="input" id="rollKiInput" inputmode="decimal" placeholder="Ki">
            <input class="input" id="rollKdInput" inputmode="decimal" placeholder="Kd">
          </div>
        </label>
        <label class="field">
          <span id="pitchPidLabel">Pitch PID</span>
          <div class="mini-grid">
            <input class="input" id="pitchKpInput" inputmode="decimal" placeholder="Kp">
            <input class="input" id="pitchKiInput" inputmode="decimal" placeholder="Ki">
            <input class="input" id="pitchKdInput" inputmode="decimal" placeholder="Kd">
          </div>
        </label>
        <label class="field">
          <span id="yawPidLabel">Yaw PID</span>
          <div class="mini-grid">
            <input class="input" id="yawKpInput" inputmode="decimal" placeholder="Kp">
            <input class="input" id="yawKiInput" inputmode="decimal" placeholder="Ki">
            <input class="input" id="yawKdInput" inputmode="decimal" placeholder="Kd">
          </div>
        </label>
      </div>
      <div class="actions">
        <button class="alt" id="saveSettingsBtn">Save PID</button>
      </div>
    </section>

    <section class="card span2 page-section" data-page="calibration">
      <h2 id="calibrationTitle">Sensor Calibration</h2>
      <div class="grid">
        <div class="meta"><div class="label">System</div><div class="value" id="imuSystemCalValue">0/3</div></div>
        <div class="meta"><div class="label">Gyro</div><div class="value" id="imuGyroCalValue">0/3</div></div>
        <div class="meta"><div class="label">Accel</div><div class="value" id="imuAccelCalValue">0/3</div></div>
        <div class="meta"><div class="label">Mag</div><div class="value" id="imuMagCalValue">0/3</div></div>
        <div class="meta"><div class="label">Calibration</div><div class="value" id="imuReadyValue">--</div></div>
        <div class="meta"><div class="label">Arming Gate</div><div class="value" id="imuGateValue">--</div></div>
      </div>
      <div class="actions">
        <button class="alt" id="calibrateBtn">Capture Level</button>
      </div>
      <p class="note" id="calibrationNote">For MPU6050 keep the frame still during boot until gyro and accel become ready. If BNO055 is connected instead, move the frame gently until calibration is ready. Then place the drone flat and press Capture Level.</p>
    </section>

    <section class="card page-section" data-page="receiver">
      <h2 id="receiverTitle">Receiver</h2>
      <div class="list" id="rcSignals"></div>
      <div class="grid" style="margin-top:12px">
        <div class="meta"><div class="label">Throttle</div><div class="value" id="throttleValue">0.0 %</div></div>
        <div class="meta"><div class="label">Arm Switch</div><div class="value" id="armSwitchValue">--</div></div>
        <div class="meta"><div class="label">Stabilize Switch</div><div class="value" id="sensorSwitchValue">--</div></div>
        <div class="meta"><div class="label">Failsafe</div><div class="value" id="failsafeValue">--</div></div>
      </div>
    </section>

    <section class="card page-section" data-page="info">
      <h2 id="wifiTitle">Wi-Fi Access</h2>
      <div class="grid">
        <div class="meta"><div class="label">Active Route</div><div class="value" id="wifiRouteValue">AP fallback</div></div>
        <div class="meta"><div class="label">Active IP</div><div class="value" id="wifiIpValue">--</div></div>
        <div class="meta"><div class="label">AP IP</div><div class="value" id="wifiApIpValue">--</div></div>
        <div class="meta"><div class="label">STA Status</div><div class="value" id="wifiStaStateValue">DISABLED</div></div>
        <div class="meta"><div class="label">STA IP</div><div class="value" id="wifiStaIpValue">--</div></div>
        <div class="meta"><div class="label">Save Method</div><div class="value" id="wifiSaveMethodValue">Edit below, then press Save Wi-Fi or Save Settings</div></div>
      </div>
      <div class="field-grid" style="margin-top:12px">
        <label class="field">
          <span>AP SSID</span>
          <input class="input" id="apSsidInput" maxlength="32" autocomplete="off">
        </label>
        <label class="field">
          <span>AP Password</span>
          <input class="input" id="apPasswordInput" maxlength="64" autocomplete="off">
        </label>
        <label class="field">
          <span>STA Backup</span>
          <div class="checkline">
            <input type="checkbox" id="staEnabledInput">
            <span>Enable router connection fallback</span>
          </div>
        </label>
        <label class="field">
          <span>STA SSID</span>
          <input class="input" id="staSsidInput" maxlength="32" autocomplete="off">
        </label>
        <label class="field">
          <span>STA Password</span>
          <input class="input" id="staPasswordInput" maxlength="64" autocomplete="off">
        </label>
      </div>
      <div class="actions">
        <button class="alt" id="saveWifiBtn">Save Wi-Fi</button>
      </div>
      <p class="note" id="wifiNote">AP stays available for direct access. STA is an optional router connection for backup access to the same web UI. Default AP password location: INPUT/drone_config.h -> kDefaultApPassword. Settings and migration live in INPUT/settings_store.cpp.</p>
    </section>

    <section class="card page-section" data-page="motors">
      <h2 id="motorsTitle">Motors</h2>
      <div class="list" id="motorSignals"></div>
      <div class="grid" style="margin-top:12px">
        <div class="meta"><div class="label">Output Driver</div><div class="value" id="motorReadyValue">--</div></div>
        <div class="meta"><div class="label">PWM Rate</div><div class="value" id="motorPwmValue">--</div></div>
        <div class="meta"><div class="label">Protocol</div><div class="value" id="motorProtoValue">PWM</div></div>
        <div class="meta"><div class="label">Pulse Range</div><div class="value" id="motorRangeValue">1000..2000 us</div></div>
      </div>
    </section>

    <section class="card span2 page-section" data-page="motors">
      <h2 id="rotationTitle">Motor Rotation Guide</h2>
      <div class="rotation-board">
        <div class="rotation-front" id="rotationFrontText">TOP VIEW / FRONT</div>
        <div class="rotation-arm a"></div>
        <div class="rotation-arm b"></div>
        <div class="rotation-core">FC</div>
        <div class="rotor m1 ccw">
          <div class="rotor-ring"></div>
          <div class="rotor-name">M1</div>
          <div class="rotor-dir">CCW</div>
        </div>
        <div class="rotor m2 cw">
          <div class="rotor-ring"></div>
          <div class="rotor-name">M2</div>
          <div class="rotor-dir">CW</div>
        </div>
        <div class="rotor m3 cw">
          <div class="rotor-ring"></div>
          <div class="rotor-name">M3</div>
          <div class="rotor-dir">CW</div>
        </div>
        <div class="rotor m4 ccw">
          <div class="rotor-ring"></div>
          <div class="rotor-name">M4</div>
          <div class="rotor-dir">CCW</div>
        </div>
      </div>
      <p class="note" id="rotationNote">Expected top-view rotation for the current mixer: M1 and M4 spin CCW, M2 and M3 spin CW. Front motors are M1 and M2.</p>
    </section>

    <section class="card page-section" data-page="pid">
      <h2 id="pidTitle">PID Tuning Snapshot</h2>
      <div class="kv" id="pidInfo"></div>
    </section>

    <section class="card page-section" data-page="info">
      <h2 id="portsTitle">Ports & Resources</h2>
      <div class="kv" id="portsInfo"></div>
    </section>

    <section class="card page-section" data-page="info">
      <h2 id="powerTitle">Power & Battery</h2>
      <div class="kv" id="powerInfo"></div>
      <p class="note" id="powerNote">No battery ADC is defined in this firmware yet, so voltage/current remain hardware TODO items for the desktop configurator phase.</p>
    </section>

    <section class="card page-section" data-page="info">
      <h2 id="featureTitle">Features & Limits</h2>
      <div class="kv" id="featureInfo"></div>
      <div class="stack" id="restrictionList" style="margin-top:12px"></div>
    </section>
  </main>

  <div class="toast" id="toast"></div>

  <script>
    const toast = document.getElementById("toast");
    const runtime = {
      tileCache: new Map(),
      lastState: null,
      pollTimer: null,
      pollMs: 450,
      heavyMs: 700,
      lastHeavyAt: 0,
      lastHeavyFetchAt: 0,
      stateFetchInFlight: null,
      heavyFetchInFlight: null,
      configFetchInFlight: null,
      networkErrorCount: 0,
      config: null,
      networkRestartUntil: 0,
      lang: localStorage.getItem("drone.ui.lang") || "en",
      page: localStorage.getItem("drone.ui.page") || "info"
    };
    const rcLabelKeys = ["rc_ch1", "rc_ch2", "rc_ch3", "rc_ch4", "rc_ch5", "rc_ch6", "rc_ch7", "rc_ch8"];
    const motorLabelKeys = ["motor_m1", "motor_m2", "motor_m3", "motor_m4"];
    const translations = {
      en: {
        title: "Drone Control Surface",
        subtitle: "Receiver map: CH1 Roll, CH2 Pitch, CH3 Throttle, CH4 Yaw. Network mode supports AP plus optional STA backup.",
        lang: "LANG",
        setup: "Setup",
        gps: "GPS",
        receiver: "Receiver",
        wifi_access: "Wi-Fi Access",
        motors: "Motors",
        motor_rotation_guide: "Motor Rotation Guide",
        pid_snapshot: "PID Tuning Snapshot",
        ports_resources: "Ports & Resources",
        power_battery: "Power & Battery",
        features_limits: "Features & Limits",
        roll: "Roll",
        pitch: "Pitch",
        yaw: "Yaw",
        arming_status: "Arming Status",
        front: "FRONT",
        rear: "REAR",
        stage_guide: "Bright nose marker = front side. Tail marker = rear side.",
        save_settings: "Save Settings",
        calibrate_level: "Calibrate Level",
        current_map_note: "Current map: CH1 Roll, CH2 Pitch, CH3 Throttle, CH4 Yaw. Motor map: M1 front-left, M2 front-right, M3 rear-left, M4 rear-right.",
        link_note_normal: "CH8 at 1000 mode: normal send/receive.",
        link_note_broadcast: "CH8 at 1500 mode: telemetry broadcast only. Incoming website commands are blocked.",
        link_note_receive: "CH8 at 2000 mode: command reception priority with reduced heavy rendering.",
        ch8_stale_suffix: "CH8 signal is stale; firmware falls back to normal command policy.",
        satellites: "Satellites",
        hdop: "HDOP",
        speed: "Speed",
        home_distance: "Home Distance",
        coordinates: "Coordinates",
        home_state: "Home State",
        set_home: "Set Home",
        clear_home: "Clear Home",
        map_note_online: "The GPS panel includes an online map when the browser still has internet access. If tiles are unreachable, it falls back to the local radar view.",
        map_note_stream_no_fix: "GPS data stream is present, but the module still has no valid sky fix. Local radar view is active until coordinates become valid.",
        map_note_no_stream: "No GPS serial data is reaching the flight controller yet. Check GPS power, TX/RX wiring, and baud rate.",
        map_note_tiles_unavailable: "Map tiles are unavailable on this network. Local radar view stays active.",
        map_note_amap: "AMAP tiles active (China-first mode with OSM fallback).",
        map_note_osm: "OSM tiles active (global mode with AMAP fallback).",
        waiting_fix: "Waiting for GPS fix",
        no_gps_stream: "No GPS stream",
        online_tiles_unavailable: "Online tiles unavailable",
        throttle: "Throttle",
        arm_switch: "Arm Switch",
        stabilize_switch: "Stabilize Switch",
        failsafe: "Failsafe",
        active_route: "Active Route",
        active_ip: "Active IP",
        ap_ip: "AP IP",
        sta_status: "STA Status",
        sta_ip: "STA IP",
        save_method: "Save Method",
        wifi_save_method: "Edit below, then press Save Wi-Fi or Save Settings",
        ap_ssid: "AP SSID",
        ap_password: "AP Password",
        sta_backup: "STA Backup",
        sta_enable_fallback: "Enable router connection fallback",
        sta_ssid: "STA SSID",
        sta_password: "STA Password",
        save_wifi: "Save Wi-Fi",
        wifi_note: "AP stays available for direct access. STA is an optional router connection for backup access to the same web UI. Router fallback is preloaded as honor / 2000320003.",
        output_driver: "Output Driver",
        pwm_rate: "PWM Rate",
        protocol: "Protocol",
        pulse_range: "Pulse Range",
        rotation_top_view: "TOP VIEW / FRONT",
        rotation_note: "Expected top-view rotation for the current mixer: M1 and M4 spin CCW, M2 and M3 spin CW. Front motors are M1 and M2.",
        power_note: "No battery ADC is defined in this firmware yet, so voltage and current remain future hardware items.",
        route_ap: "AP fallback",
        route_sta: "STA router link",
        sta_connected: "CONNECTED",
        sta_trying: "TRYING / RETRYING",
        sta_disabled: "DISABLED",
        home_set: "SET",
        home_clear: "CLEAR",
        on: "ON",
        off: "OFF",
        stale: "STALE",
        active: "ACTIVE",
        ready: "READY",
        error: "ERROR",
        safe: "SAFE",
        arm_armed: "ARM ARMED",
        arm_safe: "ARM SAFE",
        mode_prefix: "MODE",
        loop_prefix: "LOOP",
        pwm_ready: "PWM READY",
        pwm_error: "PWM ERROR",
        link_normal_chip: "LINK NORMAL",
        link_broadcast_chip: "LINK TX ONLY",
        link_receive_chip: "LINK RX PRIORITY",
        warnings: "warnings",
        done: "Done",
        toast_settings_saved: "Settings saved",
        toast_wifi_restarting: "Wi-Fi saved. The network is restarting. Reconnect if the page pauses.",
        unexpected_server_response: "Unexpected server response",
        request_failed: "Request failed",
        connection_timeout: "Connection timeout",
        invalid_numeric_value: "Invalid numeric value",
        server_no_valid_pid_fields: "No valid PID fields were provided",
        server_no_valid_writable_fields: "No valid writable fields were provided",
        server_empty_pid_payload: "Empty PID payload",
        server_pid_payload_object: "PID payload must be an object",
        server_invalid_json: "Invalid JSON",
        server_ch8_blocked: "CH8 is in 1500 broadcast-only mode. Incoming commands are blocked.",
        server_disarm_before_save: "Disarm before saving settings",
        server_disarm_before_calibration: "Disarm before calibration",
        server_imu_not_ready: "IMU is not ready",
        server_imu_wait_calibration: "Wait until IMU calibration is ready before level trim",
        server_failed_capture_samples: "Failed to capture stable IMU samples",
        server_home_updated: "Home updated",
        server_home_cleared: "Home cleared",
        server_not_found: "Not found",
        mode_manual: "MANUAL",
        mode_stabilize: "STABILIZE",
        rc_ch1: "CH1 ROLL",
        rc_ch2: "CH2 PITCH",
        rc_ch3: "CH3 THR",
        rc_ch4: "CH4 YAW",
        rc_ch5: "CH5 ARM",
        rc_ch6: "CH6 STAB",
        rc_ch7: "CH7 AUX1",
        rc_ch8: "CH8 LINK",
        motor_m1: "M1 FL",
        motor_m2: "M2 FR",
        motor_m3: "M3 RL",
        motor_m4: "M4 RR",
        kv_roll: "ROLL",
        kv_pitch: "PITCH",
        kv_limits: "LIMITS",
        kv_mixer: "MIXER",
        kv_motor_base: "MOTOR BASE",
        kv_trims: "TRIMS",
        kv_board: "BOARD",
        kv_motors: "MOTORS",
        kv_receiver: "RECEIVER",
        kv_rx_map: "RX MAP",
        kv_gps: "GPS",
        kv_i2c: "I2C",
        kv_tft: "TFT",
        kv_esc_bec: "ESC BEC",
        kv_battery_adc: "BATTERY ADC",
        kv_current_adc: "CURRENT ADC",
        kv_loop: "LOOP",
        kv_web: "WEB",
        kv_wifi: "WIFI",
        kv_failsafe: "FAILSAFE",
        kv_battery: "BATTERY",
        kv_led_strip: "LED STRIP",
        kv_blackbox: "BLACKBOX",
        kv_cli: "CLI",
        writable: "Writable",
        readonly_web: "Read-only by CH8 link policy",
        not_available: "Not available",
        battery_enabled: "Enabled",
        assigned: "Assigned",
        no_gpio: "No GPIO assigned",
        blackbox_disabled: "Disabled because SD pins are reused by ESC",
        cli_web_note: "Use USB serial and a desktop app later",
        wifi_feature_value: "AP stays on. Optional STA backup can connect through a router and serve the same web UI.",
        restriction_camera: "Disconnect the camera during flight because GPIO4..18 are reused.",
        restriction_sd: "Keep the SD slot empty because GPIO37..40 drive the ESC outputs.",
        restriction_bec: "Feed board 5V from one ESC BEC only and isolate the other red wires.",
        restriction_psram: "This build targets the no-PSRAM ESP32-S3 profile.",
        page_info: "Info",
        page_pid: "PID",
        page_visual: "3D",
        page_calibration: "Calibration",
        page_receiver: "Receiver",
        page_motors: "Motors",
        overview_title: "Drone Information",
        overview_note: "Battery voltage stays unavailable until an ADC pin is assigned in firmware. CPU usage is the estimated control-loop load.",
        battery_voltage: "Battery Voltage",
        cpu_usage: "CPU Usage",
        wifi_signal: "Wi-Fi Signal",
        flight_mode: "Flight Mode",
        system_status: "System Status",
        uptime: "Uptime",
        gps_fix_label: "GPS Fix",
        web_link: "Web Link",
        gps_fix_offline: "OFFLINE",
        gps_fix_search: "SEARCH",
        gps_fix_lock: "FIXED",
        pid_editor_title: "PID Tuning",
        save_pid: "Save PID",
        roll_pid: "Roll PID",
        pitch_pid: "Pitch PID",
        yaw_pid: "Yaw PID",
        calibration_title: "Sensor Calibration",
        calibration_state: "Calibration",
        arming_gate: "Arming Gate",
        calibration_note: "For MPU6050 keep the frame still during boot until gyro and accel become ready. If BNO055 is connected instead, move the frame gently until calibration is ready. Then place the drone flat and press Capture Level.",
        capture_level: "Capture Level",
        imu_system: "System",
        imu_gyro: "Gyro",
        imu_accel: "Accel",
        imu_mag: "Mag",
        system_ready: "READY",
        system_armed: "ARMED",
        system_wait_imu: "WAIT IMU",
        system_wait_rc: "RC FAILSAFE",
        system_wait_gps: "WAIT GPS",
        battery_unavailable: "Not available",
        calibration_pending: "IN PROGRESS",
        calibration_complete: "COMPLETE"
      },
      ar: {
        title: "واجهة تحكم الدرون",
        subtitle: "خريطة الريموت: CH1 يمين/يسار، CH2 أمام/خلف، CH3 خنق، CH4 انعراج. الشبكة تدعم نقطة الوصول AP مع اتصال STA احتياطي.",
        lang: "اللغة",
        setup: "التحكم",
        gps: "GPS",
        receiver: "المستقبل",
        wifi_access: "اتصال الواي فاي",
        motors: "المحركات",
        motor_rotation_guide: "اتجاه دوران المحركات",
        pid_snapshot: "ملخص PID",
        ports_resources: "المنافذ والموارد",
        power_battery: "الطاقة والبطارية",
        features_limits: "الميزات والحدود",
        roll: "يمين/يسار",
        pitch: "أمام/خلف",
        yaw: "Yaw",
        arming_status: "حالة التسليح",
        front: "الأمام",
        rear: "الخلف",
        stage_guide: "المؤشر المضيء هو جهة الأمام، والشريط السفلي هو جهة الخلف.",
        save_settings: "حفظ الإعدادات",
        calibrate_level: "معايرة المستوى",
        current_map_note: "الخريطة الحالية: CH1 يمين/يسار، CH2 أمام/خلف، CH3 خنق، CH4 Yaw. خريطة المحركات: M1 أمامي يسار، M2 أمامي يمين، M3 خلفي يسار، M4 خلفي يمين.",
        link_note_normal: "وضع CH8 عند 1000: إرسال واستقبال طبيعي.",
        link_note_broadcast: "وضع CH8 عند 1500: بث تليمترية فقط، وأوامر الويب محجوبة.",
        link_note_receive: "وضع CH8 عند 2000: أولوية للاستقبال مع تخفيف التحديثات الثقيلة.",
        ch8_stale_suffix: "إشارة CH8 قديمة، لذلك يعود النظام لسياسة الأوامر العادية.",
        satellites: "الأقمار",
        hdop: "HDOP",
        speed: "السرعة",
        home_distance: "المسافة للمنزل",
        coordinates: "الإحداثيات",
        home_state: "حالة المنزل",
        set_home: "تثبيت المنزل",
        clear_home: "مسح المنزل",
        map_note_online: "لوحة GPS تعرض خريطة إن كان المتصفح ما زال يملك إنترنت. عند فشل البلاطات يتم الرجوع للرادار المحلي.",
        map_note_stream_no_fix: "بيانات GPS وصلت، لكن لا يوجد تثبيت أقمار صالح بعد. الرادار المحلي يعمل حتى تصبح الإحداثيات صالحة.",
        map_note_no_stream: "لا توجد بيانات GPS تسلسلية تصل للمتحكم بعد. افحص التغذية وتوصيل TX/RX والسرعة.",
        map_note_tiles_unavailable: "بلاطات الخريطة غير متاحة على هذه الشبكة. سيبقى الرادار المحلي فعالًا.",
        map_note_amap: "خريطة AMAP فعالة مع بديل OSM.",
        map_note_osm: "خريطة OSM فعالة مع بديل AMAP.",
        waiting_fix: "بانتظار تثبيت GPS",
        no_gps_stream: "لا يوجد تدفق GPS",
        online_tiles_unavailable: "بلاطات الخريطة غير متاحة",
        throttle: "الخنق",
        arm_switch: "مفتاح التسليح",
        stabilize_switch: "مفتاح التوازن",
        failsafe: "فقد الإشارة",
        active_route: "المسار النشط",
        active_ip: "العنوان النشط",
        ap_ip: "عنوان AP",
        sta_status: "حالة STA",
        sta_ip: "عنوان STA",
        save_method: "طريقة الحفظ",
        wifi_save_method: "عدّل القيم ثم اضغط حفظ الواي فاي أو حفظ الإعدادات",
        ap_ssid: "اسم AP",
        ap_password: "كلمة مرور AP",
        sta_backup: "نسخة STA احتياطية",
        sta_enable_fallback: "تفعيل الاتصال الاحتياطي عبر الراوتر",
        sta_ssid: "اسم STA",
        sta_password: "كلمة مرور STA",
        save_wifi: "حفظ الواي فاي",
        wifi_note: "شبكة AP تبقى فعالة للدخول المباشر. اتصال STA هو مسار احتياطي للراوتر لنفس الواجهة. تم تحميل honor / 2000320003 افتراضيًا.",
        output_driver: "مشغل الخرج",
        pwm_rate: "تردد PWM",
        protocol: "البروتوكول",
        pulse_range: "مدى النبضة",
        rotation_top_view: "منظر علوي / الأمام",
        rotation_note: "الدوران المتوقع للمكسر الحالي: M1 و M4 عكس عقارب الساعة، M2 و M3 مع عقارب الساعة. المحركات الأمامية هي M1 و M2.",
        power_note: "لا يوجد ADC للبطارية في هذا الفيرموير حاليًا، لذلك الجهد والتيار مؤجلان لمرحلة عتاد لاحقة.",
        route_ap: "رجوع إلى AP",
        route_sta: "ربط الراوتر STA",
        sta_connected: "متصل",
        sta_trying: "يحاول / يعيد المحاولة",
        sta_disabled: "معطل",
        home_set: "محدد",
        home_clear: "فارغ",
        on: "ON",
        off: "OFF",
        stale: "قديم",
        active: "نشط",
        ready: "جاهز",
        error: "خطأ",
        safe: "آمن",
        arm_armed: "مسلح",
        arm_safe: "غير مسلح",
        mode_prefix: "الوضع",
        loop_prefix: "الحلقة",
        pwm_ready: "PWM جاهز",
        pwm_error: "خطأ PWM",
        link_normal_chip: "رابط طبيعي",
        link_broadcast_chip: "بث فقط",
        link_receive_chip: "أولوية استقبال",
        warnings: "تحذيرات",
        done: "تم",
        toast_settings_saved: "تم حفظ الإعدادات",
        toast_wifi_restarting: "تم حفظ الواي فاي. الشبكة تعيد التشغيل. أعد الاتصال إذا توقفت الصفحة.",
        unexpected_server_response: "استجابة غير متوقعة من الخادم",
        request_failed: "فشل الطلب",
        connection_timeout: "انتهت مهلة الاتصال",
        invalid_numeric_value: "قيمة رقمية غير صالحة",
        server_no_valid_pid_fields: "لم يتم إرسال حقول PID صالحة",
        server_no_valid_writable_fields: "لم يتم إرسال حقول قابلة للحفظ",
        server_empty_pid_payload: "بيانات PID فارغة",
        server_pid_payload_object: "صيغة PID يجب أن تكون كائنًا",
        server_invalid_json: "تنسيق JSON غير صالح",
        server_ch8_blocked: "CH8 على وضع 1500 للقراءة فقط، أوامر الويب محجوبة",
        server_disarm_before_save: "ألغِ التسليح قبل حفظ الإعدادات",
        server_disarm_before_calibration: "ألغِ التسليح قبل المعايرة",
        server_imu_not_ready: "مستشعر IMU غير جاهز",
        server_imu_wait_calibration: "انتظر جاهزية معايرة IMU قبل حفظ المستوى",
        server_failed_capture_samples: "فشل التقاط عينات IMU ثابتة",
        server_home_updated: "تم تحديث نقطة المنزل",
        server_home_cleared: "تم مسح نقطة المنزل",
        server_not_found: "المسار غير موجود",
        mode_manual: "يدوي",
        mode_stabilize: "توازن",
        rc_ch1: "CH1 يمين/يسار",
        rc_ch2: "CH2 أمام/خلف",
        rc_ch3: "CH3 خنق",
        rc_ch4: "CH4 Yaw",
        rc_ch5: "CH5 تسليح",
        rc_ch6: "CH6 توازن",
        rc_ch7: "CH7 إضافي1",
        rc_ch8: "CH8 رابط",
        motor_m1: "M1 أمامي يسار",
        motor_m2: "M2 أمامي يمين",
        motor_m3: "M3 خلفي يسار",
        motor_m4: "M4 خلفي يمين",
        kv_roll: "ROLL",
        kv_pitch: "PITCH",
        kv_limits: "الحدود",
        kv_mixer: "المكسر",
        kv_motor_base: "أساس المحرك",
        kv_trims: "الترميم",
        kv_board: "اللوحة",
        kv_motors: "المحركات",
        kv_receiver: "المستقبل",
        kv_rx_map: "خريطة القنوات",
        kv_gps: "GPS",
        kv_i2c: "I2C",
        kv_tft: "TFT",
        kv_esc_bec: "ESC BEC",
        kv_battery_adc: "Battery ADC",
        kv_current_adc: "Current ADC",
        kv_loop: "الحلقات",
        kv_web: "الويب",
        kv_wifi: "الواي فاي",
        kv_failsafe: "فقد الإشارة",
        kv_battery: "البطارية",
        kv_led_strip: "شريط LED",
        kv_blackbox: "بلاك بوكس",
        kv_cli: "CLI",
        writable: "قابل للكتابة",
        readonly_web: "قراءة فقط حسب سياسة CH8",
        not_available: "غير متوفر",
        battery_enabled: "مفعل",
        assigned: "معين",
        no_gpio: "لا يوجد GPIO",
        blackbox_disabled: "معطل لأن منافذ SD مستخدمة للمحركات",
        cli_web_note: "استخدم USB وبرنامج سطح مكتب لاحقًا",
        wifi_feature_value: "شبكة AP تبقى عاملة، ويمكن لـ STA أن تتصل بالراوتر وتقدم نفس الواجهة.",
        restriction_camera: "افصل الكاميرا أثناء الطيران لأن GPIO4..18 معاد استخدامها.",
        restriction_sd: "اترك منفذ SD فارغًا لأن GPIO37..40 تقود مخارج ESC.",
        restriction_bec: "غذِّ اللوحة من BEC واحد فقط واعزل الأسلاك الحمراء الأخرى.",
        restriction_psram: "هذا البناء مخصص لوضع ESP32-S3 بدون PSRAM.",
        page_info: "المعلومات",
        page_pid: "PID",
        page_visual: "3D",
        page_calibration: "المعايرة",
        page_receiver: "المستقبل",
        page_motors: "المحركات",
        overview_title: "معلومات الدرون",
        overview_note: "يبقى جهد البطارية غير متاح حتى يتم تعيين دبوس ADC داخل الفيرموير. استخدام المعالج هنا هو حمل حلقة التحكم التقديري.",
        battery_voltage: "جهد البطارية",
        cpu_usage: "استخدام المعالج",
        wifi_signal: "إشارة الواي فاي",
        flight_mode: "وضع الطيران",
        system_status: "حالة النظام",
        uptime: "مدة التشغيل",
        gps_fix_label: "تثبيت GPS",
        web_link: "رابط الويب",
        gps_fix_offline: "غير متصل",
        gps_fix_search: "يبحث",
        gps_fix_lock: "مثبت",
        pid_editor_title: "ضبط PID",
        save_pid: "حفظ PID",
        roll_pid: "PID يمين/يسار",
        pitch_pid: "PID أمام/خلف",
        yaw_pid: "PID Yaw",
        calibration_title: "معايرة المستشعر",
        calibration_state: "المعايرة",
        arming_gate: "بوابة التسليح",
        calibration_note: "مع MPU6050 اترك الهيكل ثابتًا أثناء الإقلاع حتى تصبح معايرة الجيروسكوب والتسارع جاهزة. وإذا كان المستشعر BNO055 موصولًا بدلًا منه فحرّك الهيكل بلطف حتى تصبح المعايرة جاهزة. بعدها ضع الدرون على سطح مستوٍ واضغط حفظ المستوى.",
        capture_level: "حفظ المستوى",
        imu_system: "النظام",
        imu_gyro: "الجيروسكوب",
        imu_accel: "التسارع",
        imu_mag: "المغناطيس",
        system_ready: "جاهز",
        system_armed: "مسلح",
        system_wait_imu: "بانتظار IMU",
        system_wait_rc: "فقد RC",
        system_wait_gps: "بانتظار GPS",
        battery_unavailable: "غير متوفر",
        calibration_pending: "قيد التنفيذ",
        calibration_complete: "مكتملة"
      },
      zh: {
        title: "无人机控制面板",
        subtitle: "遥控映射: CH1 横滚, CH2 俯仰, CH3 油门, CH4 偏航。网络支持 AP 和可选 STA 备份。",
        lang: "语言",
        setup: "姿态控制",
        gps: "GPS",
        receiver: "接收机",
        wifi_access: "Wi-Fi 连接",
        motors: "电机",
        motor_rotation_guide: "电机旋转说明",
        pid_snapshot: "PID 快照",
        ports_resources: "端口与资源",
        power_battery: "电源与电池",
        features_limits: "功能与限制",
        roll: "横滚",
        pitch: "俯仰",
        yaw: "偏航",
        arming_status: "解锁状态",
        front: "前方",
        rear: "后方",
        stage_guide: "亮色机头代表前方，底部横条代表后方。",
        save_settings: "保存设置",
        calibrate_level: "水平校准",
        current_map_note: "当前映射: CH1 横滚, CH2 俯仰, CH3 油门, CH4 偏航。电机映射: M1 左前, M2 右前, M3 左后, M4 右后。",
        link_note_normal: "CH8=1000: 正常收发。",
        link_note_broadcast: "CH8=1500: 仅遥测广播，网页命令被阻止。",
        link_note_receive: "CH8=2000: 接收优先，重绘频率降低。",
        ch8_stale_suffix: "CH8 信号过旧，固件已回退到正常命令策略。",
        satellites: "卫星数",
        hdop: "HDOP",
        speed: "速度",
        home_distance: "返航距离",
        coordinates: "坐标",
        home_state: "Home 状态",
        set_home: "设为 Home",
        clear_home: "清除 Home",
        map_note_online: "如果浏览器仍有外网，GPS 面板会显示在线地图；否则自动回退到本地雷达图。",
        map_note_stream_no_fix: "GPS 串流已到达，但还没有有效定位，当前显示本地雷达图。",
        map_note_no_stream: "飞控尚未收到 GPS 串口数据，请检查供电、TX/RX 接线和波特率。",
        map_note_tiles_unavailable: "当前网络无法加载地图瓦片，本地雷达图保持启用。",
        map_note_amap: "已启用高德地图，OSM 作为备用。",
        map_note_osm: "已启用 OSM 地图，高德作为备用。",
        waiting_fix: "等待 GPS 定位",
        no_gps_stream: "没有 GPS 数据流",
        online_tiles_unavailable: "在线瓦片不可用",
        throttle: "油门",
        arm_switch: "解锁开关",
        stabilize_switch: "平衡开关",
        failsafe: "失控保护",
        active_route: "当前链路",
        active_ip: "当前 IP",
        ap_ip: "AP IP",
        sta_status: "STA 状态",
        sta_ip: "STA IP",
        save_method: "保存方式",
        wifi_save_method: "先修改下方内容，再点击保存 Wi-Fi 或保存设置",
        ap_ssid: "AP 名称",
        ap_password: "AP 密码",
        sta_backup: "STA 备份",
        sta_enable_fallback: "启用路由器备份连接",
        sta_ssid: "STA 名称",
        sta_password: "STA 密码",
        save_wifi: "保存 Wi-Fi",
        wifi_note: "AP 一直保持可直连。STA 是同一网页的路由器备份链路，已预置 honor / 2000320003。",
        output_driver: "输出驱动",
        pwm_rate: "PWM 频率",
        protocol: "协议",
        pulse_range: "脉宽范围",
        rotation_top_view: "顶视图 / 前方",
        rotation_note: "当前混控的旋转方向: M1 与 M4 逆时针，M2 与 M3 顺时针。前方电机为 M1 和 M2。",
        power_note: "当前固件尚未接入电池 ADC，因此电压和电流仍属于后续硬件项目。",
        route_ap: "AP 直连",
        route_sta: "STA 路由器链路",
        sta_connected: "已连接",
        sta_trying: "连接中 / 重试中",
        sta_disabled: "已禁用",
        home_set: "已设定",
        home_clear: "空",
        on: "开",
        off: "关",
        stale: "过旧",
        active: "激活",
        ready: "就绪",
        error: "错误",
        safe: "安全",
        arm_armed: "已解锁",
        arm_safe: "未解锁",
        mode_prefix: "模式",
        loop_prefix: "循环",
        pwm_ready: "PWM 就绪",
        pwm_error: "PWM 错误",
        link_normal_chip: "正常链路",
        link_broadcast_chip: "仅发射",
        link_receive_chip: "接收优先",
        warnings: "警告",
        done: "完成",
        toast_settings_saved: "设置已保存",
        toast_wifi_restarting: "Wi-Fi 已保存，网络正在重启。如页面暂停，请重新连接。",
        unexpected_server_response: "服务器响应异常",
        request_failed: "请求失败",
        connection_timeout: "连接超时",
        invalid_numeric_value: "数值格式无效",
        server_no_valid_pid_fields: "未提供有效的 PID 字段",
        server_no_valid_writable_fields: "未提供可写字段",
        server_empty_pid_payload: "PID 数据为空",
        server_pid_payload_object: "PID 数据必须是对象",
        server_invalid_json: "JSON 格式无效",
        server_ch8_blocked: "CH8 处于 1500 只读模式，网页命令被阻止",
        server_disarm_before_save: "保存设置前请先解除解锁",
        server_disarm_before_calibration: "校准前请先解除解锁",
        server_imu_not_ready: "IMU 尚未就绪",
        server_imu_wait_calibration: "请等待 IMU 校准完成后再保存水平",
        server_failed_capture_samples: "获取稳定 IMU 样本失败",
        server_home_updated: "Home 点已更新",
        server_home_cleared: "Home 点已清除",
        server_not_found: "未找到该接口",
        mode_manual: "手动",
        mode_stabilize: "平衡",
        rc_ch1: "CH1 横滚",
        rc_ch2: "CH2 俯仰",
        rc_ch3: "CH3 油门",
        rc_ch4: "CH4 偏航",
        rc_ch5: "CH5 解锁",
        rc_ch6: "CH6 平衡",
        rc_ch7: "CH7 辅助1",
        rc_ch8: "CH8 链路",
        motor_m1: "M1 左前",
        motor_m2: "M2 右前",
        motor_m3: "M3 左后",
        motor_m4: "M4 右后",
        kv_roll: "ROLL",
        kv_pitch: "PITCH",
        kv_limits: "限制",
        kv_mixer: "混控",
        kv_motor_base: "电机基础",
        kv_trims: "微调",
        kv_board: "主板",
        kv_motors: "电机",
        kv_receiver: "接收机",
        kv_rx_map: "通道映射",
        kv_gps: "GPS",
        kv_i2c: "I2C",
        kv_tft: "TFT",
        kv_esc_bec: "ESC BEC",
        kv_battery_adc: "电池 ADC",
        kv_current_adc: "电流 ADC",
        kv_loop: "循环",
        kv_web: "网页",
        kv_wifi: "Wi-Fi",
        kv_failsafe: "失控保护",
        kv_battery: "电池",
        kv_led_strip: "LED 灯带",
        kv_blackbox: "黑盒",
        kv_cli: "CLI",
        writable: "可写",
        readonly_web: "受 CH8 策略限制为只读",
        not_available: "不可用",
        battery_enabled: "已启用",
        assigned: "已分配",
        no_gpio: "未分配 GPIO",
        blackbox_disabled: "因 SD 引脚被 ESC 占用而禁用",
        cli_web_note: "后续可使用 USB 串口与桌面工具",
        wifi_feature_value: "AP 始终开启，STA 可通过路由器连接并访问同一网页界面。",
        restriction_camera: "飞行时请断开摄像头，因为 GPIO4..18 已复用。",
        restriction_sd: "请勿插入 SD 卡，因为 GPIO37..40 用于 ESC 输出。",
        restriction_bec: "只使用一个 ESC 的 BEC 给飞控供电，其余红线必须隔离。",
        restriction_psram: "此版本针对禁用 PSRAM 的 ESP32-S3 配置。",
        page_info: "信息",
        page_pid: "PID",
        page_visual: "3D",
        page_calibration: "校准",
        page_receiver: "接收机",
        page_motors: "电机",
        overview_title: "无人机信息",
        overview_note: "在固件分配 ADC 引脚之前，电池电压保持不可用。CPU 使用率显示的是控制环路估算负载。",
        battery_voltage: "电池电压",
        cpu_usage: "CPU 使用率",
        wifi_signal: "Wi-Fi 信号",
        flight_mode: "飞行模式",
        system_status: "系统状态",
        uptime: "运行时间",
        gps_fix_label: "GPS 定位",
        web_link: "网页链路",
        gps_fix_offline: "离线",
        gps_fix_search: "搜索中",
        gps_fix_lock: "已定位",
        pid_editor_title: "PID 调参",
        save_pid: "保存 PID",
        roll_pid: "横滚 PID",
        pitch_pid: "俯仰 PID",
        yaw_pid: "偏航 PID",
        calibration_title: "传感器校准",
        calibration_state: "校准状态",
        arming_gate: "解锁门限",
        calibration_note: "使用 MPU6050 时，请在启动期间保持机体静止，直到陀螺仪和加速度计就绪。如果接的是 BNO055，请轻轻移动机体直到校准就绪。然后将无人机放平并点击保存水平。",
        capture_level: "保存水平",
        imu_system: "系统",
        imu_gyro: "陀螺仪",
        imu_accel: "加速度计",
        imu_mag: "磁力计",
        system_ready: "就绪",
        system_armed: "已解锁",
        system_wait_imu: "等待 IMU",
        system_wait_rc: "RC 失控",
        system_wait_gps: "等待 GPS",
        battery_unavailable: "不可用",
        calibration_pending: "校准中",
        calibration_complete: "已完成"
      }
    };

    function tr(key) {
      const langTable = translations[runtime.lang] || translations.en;
      return langTable[key] || translations.en[key] || key;
    }

    function localizeServerMessage(message) {
      if (!message) return tr("request_failed");
      const text = String(message);
      if (text === "No valid PID fields were provided") return tr("server_no_valid_pid_fields");
      if (text === "No valid writable fields were provided") return tr("server_no_valid_writable_fields");
      if (text === "Empty PID payload") return tr("server_empty_pid_payload");
      if (text === "PID payload must be an object") return tr("server_pid_payload_object");
      if (text === "CH8 is in 1500 broadcast-only mode. Incoming commands are blocked.") return tr("server_ch8_blocked");
      if (text === "Disarm before saving settings") return tr("server_disarm_before_save");
      if (text === "Disarm before calibration") return tr("server_disarm_before_calibration");
      if (text === "IMU is not ready") return tr("server_imu_not_ready");
      if (text === "Wait until IMU calibration is ready before level trim") return tr("server_imu_wait_calibration");
      if (text === "Failed to capture stable IMU samples") return tr("server_failed_capture_samples");
      if (text === "Home updated") return tr("server_home_updated");
      if (text === "Home cleared") return tr("server_home_cleared");
      if (text === "Not found") return tr("server_not_found");
      if (text === "Settings saved") return tr("toast_settings_saved");
      if (text.startsWith("Invalid JSON:")) {
        const detail = text.substring("Invalid JSON:".length).trim();
        return tr("server_invalid_json") + (detail ? ": " + detail : "");
      }
      return text;
    }

    function setNodeText(id, value) {
      const node = document.getElementById(id);
      if (node) node.textContent = value;
    }

    function setValueLabel(valueId, label) {
      const node = document.getElementById(valueId);
      if (node && node.previousElementSibling) {
        node.previousElementSibling.textContent = label;
      }
    }

    function setFieldTitle(inputId, label) {
      const field = document.getElementById(inputId);
      if (!field) return;
      const root = field.closest(".field");
      if (!root) return;
      const title = root.querySelector("span");
      if (title) title.textContent = label;
    }

    function setChecklineTitle(inputId, titleText, detailText) {
      const field = document.getElementById(inputId);
      if (!field) return;
      const root = field.closest(".field");
      if (!root) return;
      const title = root.querySelector(":scope > span");
      if (title) title.textContent = titleText;
      const detail = root.querySelector(".checkline span");
      if (detail) detail.textContent = detailText;
    }

    function translateMode(mode) {
      const key = String(mode || "").toLowerCase();
      if (key === "manual") return tr("mode_manual");
      if (key === "stabilize") return tr("mode_stabilize");
      return mode || "--";
    }

    function setPage(page) {
      runtime.page = page || "info";
      localStorage.setItem("drone.ui.page", runtime.page);
      document.querySelectorAll(".page-section").forEach(node => {
        node.hidden = node.getAttribute("data-page") !== runtime.page;
      });
      document.querySelectorAll(".page-btn").forEach(node => {
        node.classList.toggle("active", node.getAttribute("data-target") === runtime.page);
      });
    }

    function uptimeText(ms) {
      const totalSeconds = Math.max(0, Math.floor((ms || 0) / 1000));
      const hours = Math.floor(totalSeconds / 3600);
      const minutes = Math.floor((totalSeconds % 3600) / 60);
      const seconds = totalSeconds % 60;
      if (hours > 0) return hours + "h " + minutes + "m";
      if (minutes > 0) return minutes + "m " + seconds + "s";
      return seconds + "s";
    }

    function gpsFixText(state) {
      if (state.gpsFix) return tr("gps_fix_lock");
      if (state.gpsOk) return tr("gps_fix_search");
      return tr("gps_fix_offline");
    }

    function systemStatusText(state) {
      if (state.armed) return tr("system_armed");
      if (!state.imuOk) return tr("system_wait_imu");
      if (state.rcFailsafe) return tr("system_wait_rc");
      if (!state.gpsOk) return tr("system_wait_gps");
      return tr("system_ready");
    }

    function readNumericInput(id, fallback) {
      const node = document.getElementById(id);
      if (!node) return fallback;
      const raw = String(node.value || "").trim();
      if (!raw.length) return fallback;
      const normalized = raw
        .replace(/[\u0660-\u0669]/g, ch => String(ch.charCodeAt(0) - 0x0660))
        .replace(/[\u06F0-\u06F9]/g, ch => String(ch.charCodeAt(0) - 0x06F0))
        .replace(/،/g, ",")
        .replace(/,/g, ".")
        .replace(/\s+/g, "");
      const value = parseFloat(normalized);
      return Number.isFinite(value) ? value : fallback;
    }

    function requireNumericInput(id, fallback, fieldLabel) {
      const node = document.getElementById(id);
      if (!node) return fallback;
      const raw = String(node.value || "").trim();
      if (!raw.length) return fallback;
      const normalized = raw
        .replace(/[\u0660-\u0669]/g, ch => String(ch.charCodeAt(0) - 0x0660))
        .replace(/[\u06F0-\u06F9]/g, ch => String(ch.charCodeAt(0) - 0x06F0))
        .replace(/،/g, ",")
        .replace(/,/g, ".")
        .replace(/\s+/g, "");
      const value = parseFloat(normalized);
      if (!Number.isFinite(value)) {
        throw new Error(tr("invalid_numeric_value") + ": " + fieldLabel);
      }
      return value;
    }

    function renderSignalRows() {
      buildSignalRows("rcSignals", rcLabelKeys.map(key => tr(key)), "rc");
      buildSignalRows("motorSignals", motorLabelKeys.map(key => tr(key)), "motor");
    }

    function applyTranslations() {
      document.title = tr("title");
      document.documentElement.lang = runtime.lang === "zh" ? "zh-CN" : runtime.lang;
      document.documentElement.dir = runtime.lang === "ar" ? "rtl" : "ltr";
      setNodeText("pageBtnInfo", tr("page_info"));
      setNodeText("pageBtnPid", tr("page_pid"));
      setNodeText("pageBtnVisual", tr("page_visual"));
      setNodeText("pageBtnCalibration", tr("page_calibration"));
      setNodeText("pageBtnReceiver", tr("page_receiver"));
      setNodeText("pageBtnMotors", tr("page_motors"));
      setNodeText("titleText", tr("title"));
      setNodeText("subtitleText", tr("subtitle"));
      setNodeText("langLabel", tr("lang"));
      setNodeText("overviewTitle", tr("overview_title"));
      setNodeText("overviewNote", tr("overview_note"));
      setNodeText("setupTitle", tr("setup"));
      setNodeText("gpsTitle", tr("gps"));
      setNodeText("receiverTitle", tr("receiver"));
      setNodeText("wifiTitle", tr("wifi_access"));
      setNodeText("motorsTitle", tr("motors"));
      setNodeText("rotationTitle", tr("motor_rotation_guide"));
      setNodeText("pidEditorTitle", tr("pid_editor_title"));
      setNodeText("saveSettingsBtn", tr("save_pid"));
      setNodeText("rollPidLabel", tr("roll_pid"));
      setNodeText("pitchPidLabel", tr("pitch_pid"));
      setNodeText("yawPidLabel", tr("yaw_pid"));
      setNodeText("calibrationTitle", tr("calibration_title"));
      setNodeText("pidTitle", tr("pid_snapshot"));
      setNodeText("portsTitle", tr("ports_resources"));
      setNodeText("powerTitle", tr("power_battery"));
      setNodeText("featureTitle", tr("features_limits"));
      setNodeText("stageFrontBadge", tr("front"));
      setNodeText("stageRearBadge", tr("rear"));
      setNodeText("stageGuideText", tr("stage_guide"));
      setNodeText("calibrateBtn", tr("capture_level"));
      setNodeText("calibrationNote", tr("calibration_note"));
      setNodeText("currentMapNote", tr("current_map_note"));
      setNodeText("mapNote", tr("map_note_online"));
      setNodeText("setHomeBtn", tr("set_home"));
      setNodeText("clearHomeBtn", tr("clear_home"));
      setNodeText("saveWifiBtn", tr("save_wifi"));
      setNodeText("wifiSaveMethodValue", tr("wifi_save_method"));
      setNodeText("wifiNote", tr("wifi_note"));
      setNodeText("rotationFrontText", tr("rotation_top_view"));
      setNodeText("rotationNote", tr("rotation_note"));
      setNodeText("powerNote", tr("power_note"));

      setValueLabel("rollValue", tr("roll"));
      setValueLabel("pitchValue", tr("pitch"));
      setValueLabel("yawValue", tr("yaw"));
      setValueLabel("armingReasonValue", tr("arming_status"));
      setValueLabel("batteryValue", tr("battery_voltage"));
      setValueLabel("cpuValue", tr("cpu_usage"));
      setValueLabel("rssiValue", tr("wifi_signal"));
      setValueLabel("flightModeValue", tr("flight_mode"));
      setValueLabel("systemStatusValue", tr("system_status"));
      setValueLabel("uptimeValue", tr("uptime"));
      setValueLabel("gpsFixValue", tr("gps_fix_label"));
      setValueLabel("linkModeValue", tr("web_link"));
      setValueLabel("gpsSat", tr("satellites"));
      setValueLabel("gpsHdop", tr("hdop"));
      setValueLabel("gpsSpeed", tr("speed"));
      setValueLabel("gpsHome", tr("home_distance"));
      setValueLabel("gpsCoord", tr("coordinates"));
      setValueLabel("gpsHomeState", tr("home_state"));
      setValueLabel("imuSystemCalValue", tr("imu_system"));
      setValueLabel("imuGyroCalValue", tr("imu_gyro"));
      setValueLabel("imuAccelCalValue", tr("imu_accel"));
      setValueLabel("imuMagCalValue", tr("imu_mag"));
      setValueLabel("imuReadyValue", tr("calibration_state"));
      setValueLabel("imuGateValue", tr("arming_gate"));
      setValueLabel("throttleValue", tr("throttle"));
      setValueLabel("armSwitchValue", tr("arm_switch"));
      setValueLabel("sensorSwitchValue", tr("stabilize_switch"));
      setValueLabel("failsafeValue", tr("failsafe"));
      setValueLabel("wifiRouteValue", tr("active_route"));
      setValueLabel("wifiIpValue", tr("active_ip"));
      setValueLabel("wifiApIpValue", tr("ap_ip"));
      setValueLabel("wifiStaStateValue", tr("sta_status"));
      setValueLabel("wifiStaIpValue", tr("sta_ip"));
      setValueLabel("wifiSaveMethodValue", tr("save_method"));
      setValueLabel("motorReadyValue", tr("output_driver"));
      setValueLabel("motorPwmValue", tr("pwm_rate"));
      setValueLabel("motorProtoValue", tr("protocol"));
      setValueLabel("motorRangeValue", tr("pulse_range"));

      setFieldTitle("apSsidInput", tr("ap_ssid"));
      setFieldTitle("apPasswordInput", tr("ap_password"));
      setChecklineTitle("staEnabledInput", tr("sta_backup"), tr("sta_enable_fallback"));
      setFieldTitle("staSsidInput", tr("sta_ssid"));
      setFieldTitle("staPasswordInput", tr("sta_password"));
    }

    function msg(text, bad) {
      toast.textContent = text;
      toast.className = "toast show" + (bad ? " bad" : "");
      clearTimeout(msg.timer);
      msg.timer = setTimeout(() => { toast.className = "toast"; }, 2200);
    }

    function txt(id, value) {
      document.getElementById(id).textContent = value;
    }

    function fmt(value, digits) {
      return Number.isFinite(value) ? value.toFixed(digits) : "--";
    }

    function buildSignalRows(containerId, names, prefix) {
      const root = document.getElementById(containerId);
      root.innerHTML = "";
      names.forEach((name, index) => {
        root.insertAdjacentHTML(
          "beforeend",
          "<div class=\"row\" id=\"" + prefix + "Row" + index + "\">" +
            "<div class=\"lab\">" + name + "</div>" +
            "<div class=\"shell\"><div class=\"fill\" id=\"" + prefix + "Fill" + index + "\"></div></div>" +
            "<div class=\"num\" id=\"" + prefix + "Value" + index + "\">0</div>" +
          "</div>"
        );
      });
    }

    function setSignal(prefix, index, value, min, max, fresh) {
      const clamped = Math.max(0, Math.min(100, ((value - min) / (max - min)) * 100));
      document.getElementById(prefix + "Fill" + index).style.width = clamped + "%";
      document.getElementById(prefix + "Value" + index).textContent = Math.round(value) + (fresh === false ? " stale" : "");
      document.getElementById(prefix + "Row" + index).classList.toggle("stale", fresh === false);
    }

    function buildKv(rootId, items) {
      const root = document.getElementById(rootId);
      root.innerHTML = items.map(item => "<div><strong>" + item[0] + "</strong>" + item[1] + "</div>").join("");
    }

    function buildRestrictionList(items) {
      const root = document.getElementById("restrictionList");
      root.innerHTML = items.map(item => "<div class=\"tag\">" + item + "</div>").join("");
    }

    function outOfChina(lat, lng) {
      return lng < 72.004 || lng > 137.8347 || lat < 0.8293 || lat > 55.8271;
    }

    function transformLat(x, y) {
      let ret = -100 + 2 * x + 3 * y + 0.2 * y * y + 0.1 * x * y + 0.2 * Math.sqrt(Math.abs(x));
      ret += (20 * Math.sin(6 * x * Math.PI) + 20 * Math.sin(2 * x * Math.PI)) * 2 / 3;
      ret += (20 * Math.sin(y * Math.PI) + 40 * Math.sin(y / 3 * Math.PI)) * 2 / 3;
      ret += (160 * Math.sin(y / 12 * Math.PI) + 320 * Math.sin(y * Math.PI / 30)) * 2 / 3;
      return ret;
    }

    function transformLng(x, y) {
      let ret = 300 + x + 2 * y + 0.1 * x * x + 0.1 * x * y + 0.1 * Math.sqrt(Math.abs(x));
      ret += (20 * Math.sin(6 * x * Math.PI) + 20 * Math.sin(2 * x * Math.PI)) * 2 / 3;
      ret += (20 * Math.sin(x * Math.PI) + 40 * Math.sin(x / 3 * Math.PI)) * 2 / 3;
      ret += (150 * Math.sin(x / 12 * Math.PI) + 300 * Math.sin(x / 30 * Math.PI)) * 2 / 3;
      return ret;
    }

    function wgs84ToGcj02(lat, lng) {
      if (outOfChina(lat, lng)) return { lat, lng };
      const a = 6378245.0;
      const ee = 0.00669342162296594323;
      let dLat = transformLat(lng - 105, lat - 35);
      let dLng = transformLng(lng - 105, lat - 35);
      const radLat = lat / 180 * Math.PI;
      let magic = Math.sin(radLat);
      magic = 1 - ee * magic * magic;
      const sqrtMagic = Math.sqrt(magic);
      dLat = (dLat * 180) / ((a * (1 - ee)) / (magic * sqrtMagic) * Math.PI);
      dLng = (dLng * 180) / (a / sqrtMagic * Math.cos(radLat) * Math.PI);
      return { lat: lat + dLat, lng: lng + dLng };
    }

    function zoomForDistance(distanceHome) {
      if (!Number.isFinite(distanceHome) || distanceHome <= 30) return 17;
      if (distanceHome <= 150) return 16;
      if (distanceHome <= 600) return 15;
      if (distanceHome <= 2000) return 14;
      return 13;
    }

    function tileUrlAmap(x, y, z) {
      const sub = ((x + y) % 4 + 4) % 4 + 1;
      return "https://wprd0" + sub + ".is.autonavi.com/appmaptile?lang=zh_cn&size=1&scl=1&style=7&x=" + x + "&y=" + y + "&z=" + z;
    }

    function tileUrlOsm(x, y, z) {
      return "https://tile.openstreetmap.org/" + z + "/" + x + "/" + y + ".png";
    }

    function tileCandidates(x, y, z, lat, lng) {
      if (outOfChina(lat, lng)) {
        return [
          { provider: "OSM", url: tileUrlOsm(x, y, z) },
          { provider: "AMAP", url: tileUrlAmap(x, y, z) }
        ];
      }
      return [
        { provider: "AMAP", url: tileUrlAmap(x, y, z) },
        { provider: "OSM", url: tileUrlOsm(x, y, z) }
      ];
    }

    function worldPixel(lat, lng, zoom) {
      const scale = 256 * Math.pow(2, zoom);
      const sinLat = Math.sin(lat * Math.PI / 180);
      return {
        x: ((lng + 180) / 360) * scale,
        y: (0.5 - Math.log((1 + sinLat) / (1 - sinLat)) / (4 * Math.PI)) * scale
      };
    }

    function getTile(url) {
      if (!runtime.tileCache.has(url)) {
        const image = new Image();
        image.crossOrigin = "anonymous";
        image.onload = () => {
          if (runtime.lastState) drawGps(runtime.lastState);
        };
        image.onerror = () => {
          image._failed = true;
          if (runtime.lastState) drawGps(runtime.lastState);
        };
        image.src = url;
        runtime.tileCache.set(url, image);
      }
      return runtime.tileCache.get(url);
    }

    function drawFallbackGps(ctx, canvas, state, message) {
      const w = canvas.width;
      const h = canvas.height;
      const cx = w / 2;
      const cy = h / 2;
      ctx.clearRect(0, 0, w, h);
      ctx.strokeStyle = "rgba(127,218,178,0.22)";
      [48, 90, 132].forEach(radius => {
        ctx.beginPath();
        ctx.arc(cx, cy, radius, 0, Math.PI * 2);
        ctx.stroke();
      });
      ctx.beginPath();
      ctx.moveTo(cx, 24);
      ctx.lineTo(cx, h - 24);
      ctx.moveTo(24, cy);
      ctx.lineTo(w - 24, cy);
      ctx.stroke();
      ctx.fillStyle = "#edf5f1";
      ctx.beginPath();
      ctx.arc(cx, cy, 7, 0, Math.PI * 2);
      ctx.fill();

      if (state.gpsFix) {
        const distance = state.gps.distanceHome || 0;
        const bearing = ((state.gps.bearingHome || 0) - (state.gps.course || 0)) * Math.PI / 180;
        const radius = Math.min(132, Math.max(20, distance / 2));
        const hx = cx + Math.sin(bearing) * radius;
        const hy = cy - Math.cos(bearing) * radius;
        ctx.fillStyle = "#f2bf6d";
        ctx.beginPath();
        ctx.arc(hx, hy, 8, 0, Math.PI * 2);
        ctx.fill();
        ctx.strokeStyle = "#f2bf6d";
        ctx.lineWidth = 2;
        ctx.beginPath();
        ctx.moveTo(cx, cy);
        ctx.lineTo(hx, hy);
        ctx.stroke();
      }

      ctx.fillStyle = "#edf5f1";
      ctx.font = "18px Consolas";
      ctx.fillText(message, 22, 30);
      if (state.gpsFix) {
        ctx.fillText("LAT " + fmt(state.gps.lat, 6), 22, h - 46);
        ctx.fillText("LON " + fmt(state.gps.lng, 6), 22, h - 22);
      }
    }

    function drawGps(state) {
      runtime.lastState = state;
      const canvas = document.getElementById("gpsCanvas");
      const ctx = canvas.getContext("2d");

      if (!state.gpsFix) {
        txt("mapNote", state.gpsOk
          ? tr("map_note_stream_no_fix")
          : tr("map_note_no_stream"));
        drawFallbackGps(ctx, canvas, state, state.gpsOk ? tr("waiting_fix") : tr("no_gps_stream"));
        return;
      }

      const zoom = zoomForDistance(state.gps.distanceHome);
      const center = wgs84ToGcj02(state.gps.lat, state.gps.lng);
      const centerWorld = worldPixel(center.lat, center.lng, zoom);
      const topLeft = {
        x: centerWorld.x - canvas.width / 2,
        y: centerWorld.y - canvas.height / 2
      };

      ctx.fillStyle = "#0b1213";
      ctx.fillRect(0, 0, canvas.width, canvas.height);

      let loaded = 0;
      let usedProvider = "";
      const tileSize = 256;
      const startX = Math.floor(topLeft.x / tileSize);
      const startY = Math.floor(topLeft.y / tileSize);
      const endX = Math.floor((topLeft.x + canvas.width) / tileSize);
      const endY = Math.floor((topLeft.y + canvas.height) / tileSize);
      const maxTile = Math.pow(2, zoom);

      for (let tileY = startY; tileY <= endY; tileY++) {
        if (tileY < 0 || tileY >= maxTile) continue;
        for (let tileX = startX; tileX <= endX; tileX++) {
          const wrappedX = ((tileX % maxTile) + maxTile) % maxTile;
          const drawX = tileX * tileSize - topLeft.x;
          const drawY = tileY * tileSize - topLeft.y;
          const candidates = tileCandidates(wrappedX, tileY, zoom, state.gps.lat, state.gps.lng);
          let selected = null;
          let provider = "";

          for (let i = 0; i < candidates.length; i++) {
            const image = getTile(candidates[i].url);
            if (image.complete && image.naturalWidth > 0 && !image._failed) {
              selected = image;
              provider = candidates[i].provider;
              break;
            }
          }

          if (!selected) {
            getTile(candidates[0].url);
            continue;
          }

          usedProvider = provider || usedProvider;
          ctx.drawImage(selected, drawX, drawY, tileSize, tileSize);
          loaded++;
        }
      }

      if (loaded === 0) {
        txt("mapNote", tr("map_note_tiles_unavailable"));
        drawFallbackGps(ctx, canvas, state, tr("online_tiles_unavailable"));
        return;
      }

      txt("mapNote", usedProvider === "AMAP"
          ? tr("map_note_amap")
          : tr("map_note_osm"));

      ctx.strokeStyle = "rgba(255,255,255,0.15)";
      ctx.lineWidth = 1;
      ctx.strokeRect(0.5, 0.5, canvas.width - 1, canvas.height - 1);

      const currentPoint = {
        x: centerWorld.x - topLeft.x,
        y: centerWorld.y - topLeft.y
      };

      if (state.gps.homeSet) {
        const home = wgs84ToGcj02(state.gps.homeLat, state.gps.homeLng);
        const homeWorld = worldPixel(home.lat, home.lng, zoom);
        const homePoint = {
          x: homeWorld.x - topLeft.x,
          y: homeWorld.y - topLeft.y
        };

        ctx.strokeStyle = "rgba(242,191,109,0.9)";
        ctx.lineWidth = 2;
        ctx.beginPath();
        ctx.moveTo(currentPoint.x, currentPoint.y);
        ctx.lineTo(homePoint.x, homePoint.y);
        ctx.stroke();

        ctx.fillStyle = "#f2bf6d";
        ctx.beginPath();
        ctx.arc(homePoint.x, homePoint.y, 7, 0, Math.PI * 2);
        ctx.fill();

        ctx.strokeStyle = "#081010";
        ctx.beginPath();
        ctx.moveTo(homePoint.x, homePoint.y - 12);
        ctx.lineTo(homePoint.x, homePoint.y + 12);
        ctx.moveTo(homePoint.x - 12, homePoint.y);
        ctx.lineTo(homePoint.x + 12, homePoint.y);
        ctx.stroke();
      }

      ctx.fillStyle = "#7fdab2";
      ctx.beginPath();
      ctx.arc(currentPoint.x, currentPoint.y, 8, 0, Math.PI * 2);
      ctx.fill();

      ctx.strokeStyle = "#081010";
      ctx.lineWidth = 2;
      ctx.beginPath();
      ctx.arc(currentPoint.x, currentPoint.y, 12, 0, Math.PI * 2);
      ctx.stroke();

      ctx.fillStyle = "rgba(7,17,18,0.78)";
      ctx.fillRect(16, 16, 280, 72);
      ctx.fillStyle = "#edf5f1";
      ctx.font = "16px Consolas";
      ctx.fillText("LAT " + fmt(state.gps.lat, 6), 28, 42);
      ctx.fillText("LON " + fmt(state.gps.lng, 6), 28, 64);
      ctx.fillText("HOME " + fmt(state.gps.distanceHome, 1) + " m", 28, 86);
    }

    function updateLinkUi(state) {
      const mode = state.webLinkMode || "normal";
      const linkChip = document.getElementById("linkChip");
      let text = tr("link_normal_chip");
      let cls = "chip good";
      let note = tr("link_note_normal");

      if (mode === "broadcast") {
        text = tr("link_broadcast_chip");
        cls = "chip bad";
        note = tr("link_note_broadcast");
      } else if (mode === "receive") {
        text = tr("link_receive_chip");
        cls = "chip warn";
        note = tr("link_note_receive");
      }

      if (!state.webLinkCh8Fresh) {
        text += " (CH8 STALE)";
        note += " " + tr("ch8_stale_suffix");
      }

      linkChip.className = cls;
      txt("linkChip", text);
      txt("linkNote", note);
    }

    function cloneArray(value, fallback) {
      if (Array.isArray(value)) return value.slice();
      return Array.isArray(fallback) ? fallback.slice() : [];
    }

    function mergeStatePatch(patch) {
      const previous = runtime.lastState || {};
      return {
        ...previous,
        ...patch,
        gps: {
          ...(previous.gps || {}),
          ...(patch.gps || {})
        },
        wifi: {
          ...(previous.wifi || {}),
          ...(patch.wifi || {})
        },
        imuCal: {
          ...(previous.imuCal || {}),
          ...(patch.imuCal || {})
        },
        rc: cloneArray(patch.rc, previous.rc),
        rcFresh: cloneArray(patch.rcFresh, previous.rcFresh),
        motors: cloneArray(patch.motors, previous.motors)
      };
    }

    function applyState(state) {
      runtime.lastState = state;
      const staConnected = !!(state.wifi && state.wifi.staConnected);
      const staEnabled = !!(state.wifi && state.wifi.staEnabled);
      const activeRoute = staConnected ? tr("route_sta") : tr("route_ap");
      const activeIp = (state.wifi && state.wifi.ip) || "--";
      txt("modeChip", tr("mode_prefix") + " " + translateMode(state.mode));
      txt("armChip", state.armed ? tr("arm_armed") : tr("arm_safe"));
      txt("loopChip", tr("loop_prefix") + " " + state.loopHz + " Hz");
      txt("wifiChip", (staConnected ? "STA " : "AP ") + activeIp);
      txt("motorChip", state.motorOutputOk ? tr("pwm_ready") : tr("pwm_error"));
      txt("wifiRouteValue", activeRoute);
      txt("wifiIpValue", activeIp);
      txt("wifiApIpValue", (state.wifi && state.wifi.apIp) || "--");
      txt("wifiStaStateValue", staConnected
        ? (tr("sta_connected") + (Number.isFinite(state.wifi.staRssi) ? "  " + state.wifi.staRssi + " dBm" : ""))
        : (staEnabled ? tr("sta_trying") : tr("sta_disabled")));
      txt("wifiStaIpValue", (state.wifi && state.wifi.staIp) || "--");
      txt("batteryValue", state.batteryAvailable ? (fmt(state.batteryVoltage, 2) + " V") : tr("battery_unavailable"));
      txt("cpuValue", fmt(state.cpuLoadPct, 1) + " %");
      txt("rssiValue", staConnected && Number.isFinite(state.wifi.staRssi) ? (state.wifi.staRssi + " dBm") : "--");
      txt("flightModeValue", translateMode(state.mode));
      txt("systemStatusValue", systemStatusText(state));
      txt("uptimeValue", uptimeText(state.uptimeMs));
      txt("gpsFixValue", gpsFixText(state));
      txt("linkModeValue", state.webLinkMode || "--");

      txt("rollValue", fmt(state.roll, 1) + " deg");
      txt("pitchValue", fmt(state.pitch, 1) + " deg");
      txt("yawValue", fmt(state.yaw, 1) + " deg");
      txt("armingReasonValue", state.armingReason || "--");
      const gps = state.gps || {};
      txt("gpsSat", String(gps.sat || 0));
      txt("gpsHdop", fmt(gps.hdop, 1));
      txt("gpsSpeed", fmt(gps.speed, 1) + " m/s");
      txt("gpsHome", fmt(gps.distanceHome, 1) + " m");
      txt("gpsCoord", state.gpsFix ? fmt(gps.lat, 6) + ", " + fmt(gps.lng, 6) : "--");
      txt("gpsHomeState", state.gpsHomeSet ? tr("home_set") : tr("home_clear"));
      txt("imuSystemCalValue", String((state.imuCal && state.imuCal.sys) || 0) + "/3");
      txt("imuGyroCalValue", String((state.imuCal && state.imuCal.gyro) || 0) + "/3");
      txt("imuAccelCalValue", String((state.imuCal && state.imuCal.accel) || 0) + "/3");
      txt("imuMagCalValue", String((state.imuCal && state.imuCal.mag) || 0) + "/3");
      txt("imuReadyValue", state.imuCalibrated ? tr("calibration_complete") : tr("calibration_pending"));
      txt("imuGateValue", state.imuCalibrated ? tr("ready") : tr("system_wait_imu"));
      txt("throttleValue", fmt(state.throttlePct, 1) + " %");
      txt("armSwitchValue", state.rcFresh[5] ? (state.armSwitch ? tr("on") : tr("off")) : tr("stale"));
      txt("sensorSwitchValue", state.rcFresh[6] ? (state.sensorSwitch ? tr("on") : tr("off")) : tr("stale"));
      txt("failsafeValue", state.rcFailsafe ? tr("active") : tr("home_clear"));
      txt("motorReadyValue", state.motorOutputOk ? tr("ready") : tr("error"));
      updateLinkUi(state);

      for (let i = 0; i < state.rc.length; i++) {
        setSignal("rc", i, state.rc[i], 1000, 2000, state.rcFresh[i]);
      }
      for (let i = 0; i < state.motors.length; i++) {
        setSignal("motor", i, state.motors[i], 1000, 2000, true);
      }

      const webRxAllowed = state.webRxAllowed !== false;
      document.getElementById("saveSettingsBtn").disabled = !runtime.config || !webRxAllowed || state.armed;
      document.getElementById("saveWifiBtn").disabled = !runtime.config || !webRxAllowed || state.armed;
      document.getElementById("calibrateBtn").disabled = !webRxAllowed || state.armed;
      document.getElementById("setHomeBtn").disabled = !webRxAllowed || !state.gpsFix;
      document.getElementById("clearHomeBtn").disabled = !webRxAllowed || !state.gpsHomeSet;
      document.getElementById("apSsidInput").disabled = !webRxAllowed || state.armed;
      document.getElementById("apPasswordInput").disabled = !webRxAllowed || state.armed;
      document.getElementById("staEnabledInput").disabled = !webRxAllowed || state.armed;
      document.getElementById("staSsidInput").disabled = !webRxAllowed || state.armed;
      document.getElementById("staPasswordInput").disabled = !webRxAllowed || state.armed;
      ["rollKpInput", "rollKiInput", "rollKdInput", "pitchKpInput", "pitchKiInput", "pitchKdInput",
       "yawKpInput", "yawKiInput", "yawKdInput"].forEach(id => {
        const node = document.getElementById(id);
        if (node) node.disabled = !webRxAllowed || state.armed;
      });

      const heavyHint = Number.isFinite(state.webHeavyHintMs) ? state.webHeavyHintMs : 250;
      runtime.heavyMs = Math.max(200, Math.min(2000, Math.round(heavyHint)));
      const now = Date.now();
      if ((now - runtime.lastHeavyAt) >= runtime.heavyMs) {
        runtime.lastHeavyAt = now;
        document.getElementById("droneModel").style.transform =
          "rotateX(" + (-state.pitch).toFixed(1) + "deg) " +
          "rotateY(" + state.roll.toFixed(1) + "deg) " +
          "rotateZ(" + state.yaw.toFixed(1) + "deg)";
        drawGps(state);
      }

    }

    function applyConfig(cfg) {
      runtime.config = cfg;
      txt("motorPwmValue", cfg.board.motorPwmHz + " Hz");
      txt("motorRangeValue", cfg.board.escMinUs + ".." + cfg.board.escMaxUs + " us");
      document.getElementById("apSsidInput").value = cfg.wifi.apSsid || "";
      document.getElementById("apPasswordInput").value = cfg.wifi.apPassword || "";
      document.getElementById("staEnabledInput").checked = !!cfg.wifi.staEnabled;
      document.getElementById("staSsidInput").value = cfg.wifi.staSsid || "";
      document.getElementById("staPasswordInput").value = cfg.wifi.staPassword || "";
      document.getElementById("rollKpInput").value = fmt(cfg.rollPid.p, 3);
      document.getElementById("rollKiInput").value = fmt(cfg.rollPid.i, 3);
      document.getElementById("rollKdInput").value = fmt(cfg.rollPid.d, 3);
      document.getElementById("pitchKpInput").value = fmt(cfg.pitchPid.p, 3);
      document.getElementById("pitchKiInput").value = fmt(cfg.pitchPid.i, 3);
      document.getElementById("pitchKdInput").value = fmt(cfg.pitchPid.d, 3);
      document.getElementById("yawKpInput").value = fmt(cfg.yawPid.p, 3);
      document.getElementById("yawKiInput").value = fmt(cfg.yawPid.i, 3);
      document.getElementById("yawKdInput").value = fmt(cfg.yawPid.d, 3);

      buildKv("pidInfo", [
        ["ROLL", "P " + fmt(cfg.rollPid.p, 2) + " | I " + fmt(cfg.rollPid.i, 2) + " | D " + fmt(cfg.rollPid.d, 2)],
        ["PITCH", "P " + fmt(cfg.pitchPid.p, 2) + " | I " + fmt(cfg.pitchPid.i, 2) + " | D " + fmt(cfg.pitchPid.d, 2)],
        ["YAW", "P " + fmt(cfg.yawPid.p, 2) + " | I " + fmt(cfg.yawPid.i, 2) + " | D " + fmt(cfg.yawPid.d, 2)],
        ["LIMITS", "Max angle " + fmt(cfg.maxAngleDeg, 1) + " deg | RC expo " + fmt(cfg.rcExpo, 2)],
        ["MIXER", "Manual " + fmt(cfg.manualMixUs, 0) + " us | Yaw " + fmt(cfg.yawMixUs, 0) + " us"],
        ["MOTOR BASE", "Idle " + cfg.motorIdleUs + " us | Weights " + cfg.motorWeight.map(v => fmt(v, 3)).join(", ")],
        ["TRIMS", cfg.motorTrimUs.join(", ")]
      ]);

      buildKv("portsInfo", [
        ["BOARD", cfg.board.name],
        ["MOTORS", "M1 front-left GPIO" + cfg.resources.motorPins[0] + " | M2 front-right GPIO" + cfg.resources.motorPins[1] + " | M3 rear-left GPIO" + cfg.resources.motorPins[2] + " | M4 rear-right GPIO" + cfg.resources.motorPins[3]],
        ["RECEIVER", (cfg.resources.receiver.protocol || "RC") + " RX GPIO" + cfg.resources.receiver.rxPin + " | " + cfg.resources.receiver.baud + " baud | " + (cfg.resources.receiver.inverted ? "inverted" : "non-inverted")],
        ["RX MAP", "CH1 Roll | CH2 Pitch | CH3 Throttle | CH4 Yaw | CH5 Arm | CH6 Stabilize | CH7 Aux1 | CH8 Web Link"],
        ["GPS", "RX GPIO" + cfg.resources.gps.rx + " | TX GPIO" + cfg.resources.gps.tx + " | auto-baud (" + cfg.resources.gps.baud + " first)"],
        ["I2C", "SCL GPIO" + cfg.resources.i2c.scl + " | SDA GPIO" + cfg.resources.i2c.sda],
        ["TFT", "RST " + cfg.resources.tft.rst + " | DC " + cfg.resources.tft.dc + " | MOSI " + cfg.resources.tft.mosi + " | SCK " + cfg.resources.tft.sck + " | CS " + cfg.resources.tft.cs + " | BL " + cfg.resources.tft.backlight]
      ]);

      buildKv("powerInfo", [
        ["ESC BEC", "Use only one 5V ESC BEC to power the board"],
        ["BATTERY ADC", "Not wired in this firmware"],
        ["CURRENT ADC", "Not wired in this firmware"],
        ["LOOP", cfg.board.controlHz + " Hz control | " + cfg.board.imuHz + " Hz IMU | " + cfg.board.gpsHz + " Hz GPS"]
      ]);

      buildKv("featureInfo", [
        ["WEB", cfg.board.readonlyWeb ? "Read-only by CH8 link policy" : "Writable"],
        ["WIFI", "AP stays on. Optional STA backup can connect through a router and serve the same web UI."],
        ["FAILSAFE", cfg.safety.failsafeMs + " ms | ARM on CH" + cfg.safety.armChannel + " >= " + cfg.safety.armThresholdUs + " us"],
        ["BATTERY", cfg.features.batterySense ? "Enabled" : "Not available"],
        ["LED STRIP", cfg.features.ledStrip ? "Assigned" : "No GPIO assigned"],
        ["BLACKBOX", cfg.features.blackbox ? "Enabled" : "Disabled because SD pins are reused by ESC"],
        ["CLI", cfg.features.cliUsb ? "Use USB serial / desktop app later" : "Not exposed on web"]
      ]);

      buildRestrictionList(cfg.restrictions || []);
    }

    function writableSettingsPayload(cfg) {
      const apSsidInput = document.getElementById("apSsidInput");
      const apPasswordInput = document.getElementById("apPasswordInput");
      const staEnabledInput = document.getElementById("staEnabledInput");
      const staSsidInput = document.getElementById("staSsidInput");
      const staPasswordInput = document.getElementById("staPasswordInput");
      return {
        rollPid: {
          p: cfg.rollPid.p,
          i: cfg.rollPid.i,
          d: cfg.rollPid.d,
          integralLimit: cfg.rollPid.integralLimit,
          outputLimit: cfg.rollPid.outputLimit
        },
        pitchPid: {
          p: cfg.pitchPid.p,
          i: cfg.pitchPid.i,
          d: cfg.pitchPid.d,
          integralLimit: cfg.pitchPid.integralLimit,
          outputLimit: cfg.pitchPid.outputLimit
        },
        yawPid: {
          p: cfg.yawPid.p,
          i: cfg.yawPid.i,
          d: cfg.yawPid.d,
          integralLimit: cfg.yawPid.integralLimit,
          outputLimit: cfg.yawPid.outputLimit
        },
        maxAngleDeg: cfg.maxAngleDeg,
        rcExpo: cfg.rcExpo,
        manualMixUs: cfg.manualMixUs,
        yawMixUs: cfg.yawMixUs,
        motorIdleUs: cfg.motorIdleUs,
        motorWeight: cfg.motorWeight,
        motorTrimUs: cfg.motorTrimUs,
        levelRollOffsetDeg: cfg.levelRollOffsetDeg,
        levelPitchOffsetDeg: cfg.levelPitchOffsetDeg,
        wifi: {
          apSsid: apSsidInput ? apSsidInput.value : cfg.wifi.apSsid,
          apPassword: apPasswordInput ? apPasswordInput.value : cfg.wifi.apPassword,
          staEnabled: staEnabledInput ? staEnabledInput.checked : !!cfg.wifi.staEnabled,
          staSsid: staSsidInput ? staSsidInput.value : cfg.wifi.staSsid,
          staPassword: staPasswordInput ? staPasswordInput.value : cfg.wifi.staPassword
        }
      };
    }

    function writableFlightPayload(cfg) {
      const rollP = requireNumericInput("rollKpInput", cfg.rollPid.p, "Roll Kp");
      const rollI = requireNumericInput("rollKiInput", cfg.rollPid.i, "Roll Ki");
      const rollD = requireNumericInput("rollKdInput", cfg.rollPid.d, "Roll Kd");
      const pitchP = requireNumericInput("pitchKpInput", cfg.pitchPid.p, "Pitch Kp");
      const pitchI = requireNumericInput("pitchKiInput", cfg.pitchPid.i, "Pitch Ki");
      const pitchD = requireNumericInput("pitchKdInput", cfg.pitchPid.d, "Pitch Kd");
      const yawP = requireNumericInput("yawKpInput", cfg.yawPid.p, "Yaw Kp");
      const yawI = requireNumericInput("yawKiInput", cfg.yawPid.i, "Yaw Ki");
      const yawD = requireNumericInput("yawKdInput", cfg.yawPid.d, "Yaw Kd");
      return {
        rollP,
        rollI,
        rollD,
        pitchP,
        pitchI,
        pitchD,
        yawP,
        yawI,
        yawD,
        rollPid: {
          p: rollP,
          i: rollI,
          d: rollD,
          integralLimit: cfg.rollPid.integralLimit,
          outputLimit: cfg.rollPid.outputLimit
        },
        pitchPid: {
          p: pitchP,
          i: pitchI,
          d: pitchD,
          integralLimit: cfg.pitchPid.integralLimit,
          outputLimit: cfg.pitchPid.outputLimit
        },
        yawPid: {
          p: yawP,
          i: yawI,
          d: yawD,
          integralLimit: cfg.yawPid.integralLimit,
          outputLimit: cfg.yawPid.outputLimit
        }
      };
    }

    function writableWifiPayload(cfg) {
      const full = writableSettingsPayload(cfg);
      return { wifi: full.wifi };
    }

    function shouldSuppressNetworkError() {
      return Date.now() < runtime.networkRestartUntil;
    }

    function wifiSettingsChanged(payload, cfg) {
      if (!payload || !payload.wifi || !cfg || !cfg.wifi) return false;
      return (payload.wifi.apSsid || "") !== (cfg.wifi.apSsid || "") ||
        (payload.wifi.apPassword || "") !== (cfg.wifi.apPassword || "") ||
        !!payload.wifi.staEnabled !== !!cfg.wifi.staEnabled ||
        (payload.wifi.staSsid || "") !== (cfg.wifi.staSsid || "") ||
        (payload.wifi.staPassword || "") !== (cfg.wifi.staPassword || "");
    }

    function clearPolling() {
      if (!runtime.pollTimer) return;
      clearTimeout(runtime.pollTimer);
      runtime.pollTimer = null;
    }

    function schedulePolling(ms) {
      const next = Math.max(350, Math.min(5000, Math.round(ms || runtime.pollMs || 450)));
      runtime.pollMs = next;
      clearPolling();
      runtime.pollTimer = setTimeout(() => loadState().catch(err => {
        if (!shouldSuppressNetworkError()) msg(err.message, true);
      }), next);
    }

    async function requestJson(url, options) {
      const req = options || {};
      const headers = Object.assign({
        "Cache-Control": "no-cache",
        "Pragma": "no-cache"
      }, req.headers || {});
      const controller = typeof AbortController === "function" ? new AbortController() : null;
      const timeoutMs = Number.isFinite(req.timeoutMs) ? req.timeoutMs : 4000;
      let timeoutHandle = null;
      if (controller) {
        timeoutHandle = setTimeout(() => controller.abort(), timeoutMs);
      }

      const finalOptions = Object.assign({}, req, {
        cache: "no-store",
        headers
      });
      delete finalOptions.timeoutMs;
      if (controller) {
        finalOptions.signal = controller.signal;
      }

      let response;
      try {
        response = await fetch(url, finalOptions);
      } catch (error) {
        if (error && error.name === "AbortError") {
          throw new Error(tr("connection_timeout"));
        }
        throw error;
      } finally {
        if (timeoutHandle) clearTimeout(timeoutHandle);
      }

      let json = null;
      try {
        json = await response.json();
      } catch (_) {
        throw new Error(tr("unexpected_server_response"));
      }
      if (!response.ok || json.ok === false) {
        throw new Error(localizeServerMessage(json.message || tr("request_failed")));
      }
      return json;
    }

    async function loadConfig() {
      if (runtime.configFetchInFlight) {
        return runtime.configFetchInFlight;
      }
      runtime.configFetchInFlight = (async () => {
        applyConfig(await requestJson("/api/settings", { timeoutMs: 5000 }));
      })();
      try {
        return await runtime.configFetchInFlight;
      } finally {
        runtime.configFetchInFlight = null;
      }
    }

    async function loadState() {
      if (runtime.stateFetchInFlight) {
        return runtime.stateFetchInFlight;
      }
      runtime.stateFetchInFlight = (async () => {
        const state = mergeStatePatch(await requestJson("/api/state", { timeoutMs: 2500 }));
        runtime.networkErrorCount = 0;
        applyState(state);
        const pollHint = Number.isFinite(state.webPollHintMs) ? state.webPollHintMs : runtime.pollMs;
        schedulePolling(pollHint);
        const now = Date.now();
        if (!runtime.heavyFetchInFlight &&
            (runtime.lastHeavyFetchAt === 0 || (now - runtime.lastHeavyFetchAt) >= runtime.heavyMs)) {
          loadHeavyState().catch(err => {
            if (!shouldSuppressNetworkError()) msg(err.message, true);
          });
        }
        return state;
      })();

      try {
        return await runtime.stateFetchInFlight;
      } catch (error) {
        runtime.networkErrorCount += 1;
        schedulePolling(Math.min(5000, Math.max(runtime.pollMs, 500) * 2));
        throw error;
      } finally {
        runtime.stateFetchInFlight = null;
      }
    }

    async function loadHeavyState() {
      if (runtime.heavyFetchInFlight) {
        return runtime.heavyFetchInFlight;
      }
      runtime.heavyFetchInFlight = (async () => {
        const heavyPatch = await requestJson("/api/state-heavy", { timeoutMs: 3500 });
        runtime.lastHeavyFetchAt = Date.now();
        const merged = mergeStatePatch(heavyPatch);
        if (runtime.lastState) {
          applyState(merged);
        } else {
          runtime.lastState = merged;
        }
        return runtime.lastState;
      })();
      try {
        return await runtime.heavyFetchInFlight;
      } finally {
        runtime.heavyFetchInFlight = null;
      }
    }

    async function postAction(url) {
      const result = await requestJson(url, { method: "POST" });
      msg(localizeServerMessage(result.message || tr("done")));
      await loadState();
      return result;
    }

    async function saveSettings() {
      if (!runtime.config) {
        await loadConfig();
      }
      const payload = writableFlightPayload(runtime.config);
      const result = await requestJson("/api/pid", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(payload)
      });
      msg(localizeServerMessage(result.message || tr("toast_settings_saved")));
      await loadConfig();
      await loadState();
    }

    async function saveWifiSettings() {
      if (!runtime.config) {
        await loadConfig();
      }
      const payload = writableWifiPayload(runtime.config);
      const wifiChanged = wifiSettingsChanged({ wifi: payload.wifi }, runtime.config);
      const result = await requestJson("/api/settings", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(payload)
      });
      if (wifiChanged) {
        runtime.networkRestartUntil = Date.now() + 8000;
        msg(tr("toast_wifi_restarting"));
        setTimeout(() => {
          loadConfig().catch(() => {});
          loadState().catch(() => {});
          loadHeavyState().catch(() => {});
        }, 2500);
        return;
      }
      msg(localizeServerMessage(result.message || tr("toast_settings_saved")));
      await loadConfig();
      await loadState();
      await loadHeavyState();
    }

renderSignalRows();

    document.querySelectorAll(".page-btn").forEach(node => {
      node.addEventListener("click", () => setPage(node.getAttribute("data-target")));
    });

    document.getElementById("setHomeBtn").onclick = () => postAction("/api/home/set").catch(err => msg(err.message, true));
    document.getElementById("clearHomeBtn").onclick = () => postAction("/api/home/clear").catch(err => msg(err.message, true));
    document.getElementById("saveSettingsBtn").onclick = () => saveSettings().catch(err => msg(err.message, true));
    document.getElementById("saveWifiBtn").onclick = () => saveWifiSettings().catch(err => {
      if (!shouldSuppressNetworkError()) msg(err.message, true);
    });
    document.getElementById("calibrateBtn").onclick = () => postAction("/api/calibrate-level")
      .then(() => loadConfig())
      .catch(err => msg(err.message, true));
document.getElementById("langSelect").addEventListener("change", function(e) {
  runtime.lang = e.target.value;
  localStorage.setItem("drone.ui.lang", runtime.lang);
  applyTranslations();
});

document.getElementById("langSelect").value = runtime.lang;
applyTranslations();
  setPage(runtime.page);
    schedulePolling(runtime.pollMs);
    loadConfig().catch(err => msg(err.message, true));
    loadState().catch(err => msg(err.message, true));
    loadHeavyState().catch(err => msg(err.message, true));
  </script>
</body>
</html>
)HTML";