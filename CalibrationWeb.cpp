#include "CalibrationWeb.h"
#include <WiFi.h>
#include <WebServer.h>
#include "MotorDriver.h"
#include "LineSensor.h"
#include "globals.h"

static WebServer s_server(80);

// ==============================================================================
// EMBEDDED DASHBOARD HTML / CSS / JS (Self-contained, no internet required)
// ==============================================================================

static const char CALIB_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Robotikka Calibration & PID</title>
<style>
  :root {
    --bg: #0f141c;
    --card: #18202c;
    --border: #263445;
    --accent: #00d2ff;
    --accent2: #00ff88;
    --warn: #ff9900;
    --danger: #ff4444;
    --text: #e6edf3;
    --text-dim: #8b949e;
  }
  * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
  body { background: var(--bg); color: var(--text); padding: 16px; max-width: 900px; margin: auto; }
  h1 { font-size: 1.4rem; color: var(--accent); display: flex; align-items: center; justify-content: space-between; margin-bottom: 4px; }
  .badge { background: #00384d; color: var(--accent); padding: 3px 8px; border-radius: 12px; font-size: 0.75rem; border: 1px solid var(--accent); }
  .subtitle { color: var(--text-dim); font-size: 0.85rem; margin-bottom: 16px; }
  .card { background: var(--card); border: 1px solid var(--border); border-radius: 10px; padding: 16px; margin-bottom: 16px; box-shadow: 0 4px 12px rgba(0,0,0,0.3); }
  .card-title { font-size: 1.05rem; font-weight: 600; margin-bottom: 12px; display: flex; justify-content: space-between; align-items: center; }
  .status-bar { background: #131c26; border-left: 4px solid var(--accent); padding: 10px 14px; border-radius: 4px; margin-bottom: 16px; font-size: 0.9rem; }
  
  /* Sensor Bars */
  .sensor-grid { display: grid; grid-template-columns: repeat(5, 1fr); gap: 8px; margin-bottom: 12px; }
  .sensor-col { background: #10161f; border: 1px solid var(--border); border-radius: 8px; padding: 10px 6px; text-align: center; }
  .sensor-label { font-size: 0.75rem; color: var(--text-dim); margin-bottom: 6px; font-weight: 600; }
  .bar-container { height: 120px; background: #0a0d12; border-radius: 6px; position: relative; overflow: hidden; margin-bottom: 8px; display: flex; align-items: flex-end; }
  .bar-fill { width: 100%; background: linear-gradient(to top, var(--accent), var(--accent2)); transition: height 0.15s ease; }
  .sensor-val { font-size: 0.85rem; font-weight: bold; }
  .sensor-tag { font-size: 0.65rem; padding: 2px 4px; border-radius: 4px; margin-top: 4px; display: inline-block; font-weight: bold; }
  .tag-black { background: var(--accent2); color: #000; }
  .tag-white { background: #263445; color: #8b949e; }
  
  /* Sample Cards */
  .step-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(180px, 1fr)); gap: 10px; margin-bottom: 14px; }
  .step-card { background: #111722; border: 1px solid var(--border); border-radius: 8px; padding: 12px; display: flex; flex-direction: column; justify-content: space-between; }
  .step-card.active { border-color: var(--accent); box-shadow: 0 0 10px rgba(0,210,255,0.2); }
  .step-card.done { border-color: var(--accent2); }
  .step-num { font-size: 0.75rem; font-weight: bold; color: var(--text-dim); margin-bottom: 4px; }
  .step-name { font-size: 0.95rem; font-weight: 600; margin-bottom: 8px; }
  .step-vals { font-family: monospace; font-size: 0.7rem; color: var(--text-dim); margin-bottom: 10px; min-height: 28px; line-height: 1.3; }
  
  /* Buttons & Controls */
  button { background: #1e293b; color: var(--text); border: 1px solid var(--border); border-radius: 6px; padding: 8px 14px; font-size: 0.85rem; cursor: pointer; font-weight: 600; transition: all 0.2s; }
  button:hover { background: #2b3a4f; border-color: var(--accent); }
  button.btn-primary { background: #007799; color: #fff; border-color: var(--accent); }
  button.btn-primary:hover { background: #0099c2; }
  button.btn-success { background: #008844; color: #fff; border-color: var(--accent2); }
  button.btn-success:hover { background: #00aa55; }
  button.btn-danger { background: #882222; color: #fff; border-color: var(--danger); }
  button.btn-danger:hover { background: #aa2a2a; }
  
  /* Toggle Switch */
  .toggle-wrap { display: flex; align-items: center; justify-content: space-between; background: #111722; padding: 10px 14px; border-radius: 6px; margin-bottom: 14px; border: 1px solid var(--border); }
  .switch { position: relative; display: inline-block; width: 44px; height: 24px; }
  .switch input { opacity: 0; width: 0; height: 0; }
  .slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #334455; transition: .3s; border-radius: 24px; }
  .slider:before { position: absolute; content: ""; height: 18px; width: 18px; left: 3px; bottom: 3px; background-color: white; transition: .3s; border-radius: 50%; }
  input:checked + .slider { background-color: var(--accent2); }
  input:checked + .slider:before { transform: translateX(20px); }
  
  /* Form Inputs */
  .form-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 12px; margin-bottom: 14px; }
  .form-group { display: flex; flex-direction: column; }
  .form-group label { font-size: 0.75rem; color: var(--text-dim); margin-bottom: 4px; font-weight: 600; }
  .form-group input { background: #0c1017; border: 1px solid var(--border); border-radius: 6px; padding: 8px 10px; color: #fff; font-size: 0.9rem; font-weight: 600; }
  .form-group input:focus { border-color: var(--accent); outline: none; }
  .btn-row { display: flex; gap: 10px; flex-wrap: wrap; margin-top: 10px; }

  /* Toast Notification */
  #toast { position: fixed; bottom: 20px; right: 20px; background: #00aa55; color: #fff; padding: 10px 18px; border-radius: 6px; font-weight: bold; opacity: 0; transition: opacity 0.3s; pointer-events: none; z-index: 1000; box-shadow: 0 4px 12px rgba(0,0,0,0.5); }
</style>
</head>
<body>

  <h1>
    Robotikka Calibration
    <span class="badge" id="net-badge">Connected</span>
  </h1>
  <div class="subtitle">Real-time 4-point IR array calibration & live PID regulation</div>

  <!-- Status Bar -->
  <div class="status-bar" id="status-box">Status: Connecting to ESP32...</div>

  <!-- SENSOR LIVE GAUGES -->
  <div class="card">
    <div class="card-title">
      Live IR Sensor Array (GPIO 2 - 6)
      <span style="font-size:0.75rem; color:var(--text-dim);">ADC1 12-bit (0 - 4095)</span>
    </div>
    <div class="sensor-grid">
      <div class="sensor-col">
        <div class="sensor-label">IR_2 (G2)</div>
        <div class="bar-container"><div class="bar-fill" id="bar-0" style="height:0%;"></div></div>
        <div class="sensor-val" id="val-0">---</div>
        <div class="sensor-tag tag-white" id="tag-0">WHITE</div>
      </div>
      <div class="sensor-col">
        <div class="sensor-label">IR_3 (G3)</div>
        <div class="bar-container"><div class="bar-fill" id="bar-1" style="height:0%;"></div></div>
        <div class="sensor-val" id="val-1">---</div>
        <div class="sensor-tag tag-white" id="tag-1">WHITE</div>
      </div>
      <div class="sensor-col">
        <div class="sensor-label">IR_4 (G4) [C]</div>
        <div class="bar-container"><div class="bar-fill" id="bar-2" style="height:0%;"></div></div>
        <div class="sensor-val" id="val-2">---</div>
        <div class="sensor-tag tag-white" id="tag-2">WHITE</div>
      </div>
      <div class="sensor-col">
        <div class="sensor-label">IR_5 (G5)</div>
        <div class="bar-container"><div class="bar-fill" id="bar-3" style="height:0%;"></div></div>
        <div class="sensor-val" id="val-3">---</div>
        <div class="sensor-tag tag-white" id="tag-3">WHITE</div>
      </div>
      <div class="sensor-col">
        <div class="sensor-label">IR_6 (G6)</div>
        <div class="bar-container"><div class="bar-fill" id="bar-4" style="height:0%;"></div></div>
        <div class="sensor-val" id="val-4">---</div>
        <div class="sensor-tag tag-white" id="tag-4">WHITE</div>
      </div>
    </div>
  </div>

  <!-- 4-SAMPLE CALIBRATION -->
  <div class="card">
    <div class="card-title">
      4-Point Array Calibration
      <span style="font-size:0.75rem; color:var(--accent2);" id="calib-step-label">Step: Ready</span>
    </div>

    <!-- Toggle move forward during Sample 4 -->
    <div class="toggle-wrap">
      <div>
        <div style="font-size:0.9rem; font-weight:600;">Sample 4: Move Forward Calibration</div>
        <div style="font-size:0.75rem; color:var(--text-dim);">Robot drives forward briefly during 4th sample to capture dynamic track reflectance</div>
      </div>
      <label class="switch">
        <input type="checkbox" id="chk-fwd" onchange="toggleMoveForward(this.checked)" checked>
        <span class="slider"></span>
      </label>
    </div>

    <!-- 4 Sample Cards -->
    <div class="step-grid">
      <div class="step-card" id="card-0">
        <div>
          <div class="step-num">SAMPLE 1</div>
          <div class="step-name">Middle (Center)</div>
          <div class="step-vals" id="sample-text-0">Pending...</div>
        </div>
        <button class="btn-primary" onclick="triggerSample(0)">Sample Center</button>
      </div>

      <div class="step-card" id="card-1">
        <div>
          <div class="step-num">SAMPLE 2</div>
          <div class="step-name">Right Shift</div>
          <div class="step-vals" id="sample-text-1">Pending...</div>
        </div>
        <button class="btn-primary" onclick="triggerSample(1)">Sample Right</button>
      </div>

      <div class="step-card" id="card-2">
        <div>
          <div class="step-num">SAMPLE 3</div>
          <div class="step-name">Left Shift</div>
          <div class="step-vals" id="sample-text-2">Pending...</div>
        </div>
        <button class="btn-primary" onclick="triggerSample(2)">Sample Left</button>
      </div>

      <div class="step-card" id="card-3">
        <div>
          <div class="step-num">SAMPLE 4</div>
          <div class="step-name">Middle (Moving)</div>
          <div class="step-vals" id="sample-text-3">Pending...</div>
        </div>
        <button class="btn-primary" onclick="triggerSample(3)">Sample Forward</button>
      </div>
    </div>

    <div class="btn-row">
      <button class="btn-success" onclick="calculateThresholds()">Calculate & Save Thresholds</button>
      <button onclick="resetCalibration()">Reset Samples</button>
      <button class="btn-danger" onclick="exitCalibration()">Exit Calibration</button>
    </div>
  </div>

  <!-- PID TUNING FORM -->
  <div class="card">
    <div class="card-title">Live PID Tuning & Speed Regulation</div>
    <form onsubmit="savePID(event)">
      <div class="form-grid">
        <div class="form-group">
          <label>Proportional Gain (Kp)</label>
          <input type="number" step="0.1" id="pid-kp" required>
        </div>
        <div class="form-group">
          <label>Integral Gain (Ki)</label>
          <input type="number" step="0.01" id="pid-ki" required>
        </div>
        <div class="form-group">
          <label>Derivative Gain (Kd)</label>
          <input type="number" step="0.1" id="pid-kd" required>
        </div>
        <div class="form-group">
          <label>Base PWM Speed (0-255)</label>
          <input type="number" id="pid-base" required>
        </div>
        <div class="form-group">
          <label>Max PWM Speed (0-255)</label>
          <input type="number" id="pid-max" required>
        </div>
        <div class="form-group">
          <label>Min Speed Limit (-255 to 255)</label>
          <input type="number" id="pid-min" required>
        </div>
        <div class="form-group">
          <label>Global Line Threshold (0-4095)</label>
          <input type="number" id="pid-thr" required>
        </div>
      </div>
      <button type="submit" class="btn-success" style="padding:10px 20px;">Save PID Parameters</button>
    </form>
  </div>

  <div id="toast">Settings Saved</div>

<script>
  let formInitialized = false;

  function showToast(msg) {
    const t = document.getElementById('toast');
    t.innerText = msg;
    t.style.opacity = 1;
    setTimeout(() => { t.style.opacity = 0; }, 2500);
  }

  function fetchStatus() {
    fetch('/status')
      .then(res => res.json())
      .then(d => {
        document.getElementById('net-badge').innerText = 'Connected';
        document.getElementById('net-badge').style.borderColor = '#00ff88';
        document.getElementById('status-box').innerText = 'Status: ' + d.status;

        // Update sensor bars
        for (let i = 0; i < 5; i++) {
          const val = d.raw[i];
          const pct = Math.min(100, Math.max(0, (val / 4095) * 100));
          document.getElementById('bar-' + i).style.height = pct + '%';
          document.getElementById('val-' + i).innerText = val;
          
          const tag = document.getElementById('tag-' + i);
          if (d.isBlack[i]) {
            tag.innerText = 'BLACK';
            tag.className = 'sensor-tag tag-black';
          } else {
            tag.innerText = 'WHITE';
            tag.className = 'sensor-tag tag-white';
          }
        }

        // Update PID inputs once on page load
        if (!formInitialized) {
          document.getElementById('pid-kp').value = d.kp;
          document.getElementById('pid-ki').value = d.ki;
          document.getElementById('pid-kd').value = d.kd;
          document.getElementById('pid-base').value = d.baseSpeed;
          document.getElementById('pid-max').value = d.maxSpeed;
          document.getElementById('pid-min').value = d.minSpeed;
          document.getElementById('pid-thr').value = d.threshold;
          document.getElementById('chk-fwd').checked = d.moveForwardEnabled;
          formInitialized = true;
        }

        // Update Sample Cards
        for (let s = 0; s < 4; s++) {
          const card = document.getElementById('card-' + s);
          const txt = document.getElementById('sample-text-' + s);
          if (d.sampleReady[s]) {
            card.className = 'step-card done';
            txt.innerText = d.samples[s].join(' | ');
          } else {
            card.className = (d.step === s) ? 'step-card active' : 'step-card';
            txt.innerText = 'Pending...';
          }
        }

        document.getElementById('calib-step-label').innerText = 'Step: ' + (d.step === 5 ? 'Calibrated' : (d.step + 1) + '/4');
      })
      .catch(err => {
        document.getElementById('net-badge').innerText = 'Offline';
        document.getElementById('net-badge').style.borderColor = '#ff4444';
      });
  }

  function triggerSample(step) {
    fetch('/sample?step=' + step)
      .then(res => res.json())
      .then(d => {
        showToast('Sample ' + (step + 1) + ' Recorded');
        fetchStatus();
      });
  }

  function toggleMoveForward(enabled) {
    fetch('/toggleMoveForward?enabled=' + (enabled ? 1 : 0))
      .then(res => res.json())
      .then(d => {
        showToast('Move forward: ' + (enabled ? 'Enabled' : 'Disabled'));
      });
  }

  function calculateThresholds() {
    fetch('/calculateThresholds')
      .then(res => res.json())
      .then(d => {
        showToast(d.message);
        document.getElementById('pid-thr').value = d.threshold;
        fetchStatus();
      });
  }

  function resetCalibration() {
    fetch('/resetCalib')
      .then(res => res.json())
      .then(d => {
        showToast('Calibration Reset');
        fetchStatus();
      });
  }

  function exitCalibration() {
    if (confirm('Exit calibration mode and shut down Wi-Fi?')) {
      fetch('/exitCalib')
        .then(() => {
          showToast('Calibration Exited');
          setTimeout(() => { location.reload(); }, 1500);
        });
    }
  }

  function savePID(e) {
    e.preventDefault();
    const kp = document.getElementById('pid-kp').value;
    const ki = document.getElementById('pid-ki').value;
    const kd = document.getElementById('pid-kd').value;
    const base = document.getElementById('pid-base').value;
    const max = document.getElementById('pid-max').value;
    const min = document.getElementById('pid-min').value;
    const thr = document.getElementById('pid-thr').value;

    const url = `/setPID?kp=${kp}&ki=${ki}&kd=${kd}&base=${base}&max=${max}&min=${min}&thr=${thr}`;
    fetch(url)
      .then(res => res.json())
      .then(d => {
        showToast('PID Parameters Updated!');
        fetchStatus();
      });
  }

  // Poll status every 500 ms
  setInterval(fetchStatus, 500);
  fetchStatus();
</script>
</body>
</html>
)rawliteral";

// ==============================================================================
// HTTP HANDLERS
// ==============================================================================

static void handleRoot() {
  s_server.send(200, "text/html", CALIB_HTML);
}

static void handleStatus() {
  LineSensorState s = readLineSensors();

  String json = "{";
  json += "\"raw\":[";
  for (int i = 0; i < LINE_SENSOR_COUNT; i++) {
    json += String(s.raw[i]) + (i < LINE_SENSOR_COUNT - 1 ? "," : "");
  }
  json += "],\"isBlack\":[";
  for (int i = 0; i < LINE_SENSOR_COUNT; i++) {
    json += (s.isBlack[i] ? "true" : "false") + String(i < LINE_SENSOR_COUNT - 1 ? "," : "");
  }
  json += "],\"kp\":" + String(g_pidConfig.Kp, 2);
  json += ",\"ki\":" + String(g_pidConfig.Ki, 3);
  json += ",\"kd\":" + String(g_pidConfig.Kd, 2);
  json += ",\"baseSpeed\":" + String(g_pidConfig.baseSpeed);
  json += ",\"maxSpeed\":" + String(g_pidConfig.maxSpeed);
  json += ",\"minSpeed\":" + String(g_pidConfig.minSpeed);
  json += ",\"threshold\":" + String(g_pidConfig.threshold);
  json += ",\"moveForwardEnabled\":" + String(g_calibMoveForwardEnabled ? "true" : "false");
  json += ",\"step\":" + String((int)g_calibCurrentStep);
  json += ",\"sampleReady\":[";
  for (int i = 0; i < CALIB_SAMPLE_COUNT; i++) {
    json += (g_calibStepReady[i] ? "true" : "false") + String(i < CALIB_SAMPLE_COUNT - 1 ? "," : "");
  }
  json += "],\"samples\":[";
  for (int step = 0; step < CALIB_SAMPLE_COUNT; step++) {
    json += "[";
    for (int i = 0; i < LINE_SENSOR_COUNT; i++) {
      json += String(g_calibSamples[step][i]) + (i < LINE_SENSOR_COUNT - 1 ? "," : "");
    }
    json += "]" + String(step < CALIB_SAMPLE_COUNT - 1 ? "," : "");
  }
  json += "],\"status\":\"" + String(g_calibStatusMsg) + "\"}";

  s_server.send(200, "application/json", json);
}

static void handleSetPID() {
  if (s_server.hasArg("kp"))   g_pidConfig.Kp = s_server.arg("kp").toFloat();
  if (s_server.hasArg("ki"))   g_pidConfig.Ki = s_server.arg("ki").toFloat();
  if (s_server.hasArg("kd"))   g_pidConfig.Kd = s_server.arg("kd").toFloat();
  if (s_server.hasArg("base")) g_pidConfig.baseSpeed = s_server.arg("base").toInt();
  if (s_server.hasArg("max"))  g_pidConfig.maxSpeed = s_server.arg("max").toInt();
  if (s_server.hasArg("min"))  g_pidConfig.minSpeed = s_server.arg("min").toInt();
  if (s_server.hasArg("thr"))  g_pidConfig.threshold = s_server.arg("thr").toInt();

  Serial.printf("[CALIB WEB] PID Updated: Kp=%.2f, Ki=%.3f, Kd=%.2f, Base=%d, Max=%d, Min=%d, Thr=%u\n",
                g_pidConfig.Kp, g_pidConfig.Ki, g_pidConfig.Kd,
                g_pidConfig.baseSpeed, g_pidConfig.maxSpeed, g_pidConfig.minSpeed,
                g_pidConfig.threshold);

  s_server.send(200, "application/json", "{\"status\":\"ok\"}");
}

static void handleSample() {
  if (s_server.hasArg("step")) {
    uint8_t step = s_server.arg("step").toInt();
    recordCalibrationSample(step);
    s_server.send(200, "application/json", "{\"status\":\"ok\",\"step\":" + String(step) + "}");
  } else {
    s_server.send(400, "application/json", "{\"error\":\"missing step arg\"}");
  }
}

static void handleToggleMoveForward() {
  if (s_server.hasArg("enabled")) {
    g_calibMoveForwardEnabled = (s_server.arg("enabled").toInt() == 1);
    Serial.printf("[CALIB WEB] Move forward calibration: %s\n",
                  g_calibMoveForwardEnabled ? "ENABLED" : "DISABLED");
    s_server.send(200, "application/json", "{\"status\":\"ok\",\"moveForward\":" + String(g_calibMoveForwardEnabled ? "true" : "false") + "}");
  } else {
    s_server.send(400, "application/json", "{\"error\":\"missing enabled arg\"}");
  }
}

static void handleCalculateThresholds() {
  bool ok = computeCalibrationThresholds();
  String json = "{\"status\":\"" + String(ok ? "ok" : "warn") + "\"";
  json += ",\"threshold\":" + String(g_pidConfig.threshold);
  json += ",\"message\":\"" + String(g_calibStatusMsg) + "\"}";
  s_server.send(200, "application/json", json);
}

static void handleResetCalib() {
  resetCalibration();
  s_server.send(200, "application/json", "{\"status\":\"ok\"}");
}

static void handleExitCalib() {
  Serial.println("[CALIB WEB] Exit requested via Web UI");
  g_runMode = MODE_IDLE;
  s_server.send(200, "application/json", "{\"status\":\"exiting\"}");
}

// ==============================================================================
// PUBLIC CALIBRATION CONTROL FUNCTIONS
// ==============================================================================

void recordCalibrationSample(uint8_t stepIndex) {
  if (stepIndex >= CALIB_SAMPLE_COUNT) return;

  if (stepIndex == 3 && g_calibMoveForwardEnabled) {
    snprintf(g_calibStatusMsg, sizeof(g_calibStatusMsg), "Sampling Step 4: Moving forward...");
    setMotorsSpeed(45); // low safe calibration speed

    // Sample continuously for ~480 ms while moving forward
    uint32_t accum[LINE_SENSOR_COUNT] = {0};
    const int rounds = 16;
    for (int r = 0; r < rounds; r++) {
      for (int i = 0; i < LINE_SENSOR_COUNT; i++) {
        accum[i] += analogRead(IR_PINS[i]);
      }
      vTaskDelay(pdMS_TO_TICKS(30));
    }
    stopMotors();

    for (int i = 0; i < LINE_SENSOR_COUNT; i++) {
      g_calibSamples[3][i] = accum[i] / rounds;
    }
  } else {
    // Stationary sample: average 12 readings over 120 ms
    uint32_t accum[LINE_SENSOR_COUNT] = {0};
    const int rounds = 12;
    for (int r = 0; r < rounds; r++) {
      for (int i = 0; i < LINE_SENSOR_COUNT; i++) {
        accum[i] += analogRead(IR_PINS[i]);
      }
      vTaskDelay(pdMS_TO_TICKS(10));
    }
    for (int i = 0; i < LINE_SENSOR_COUNT; i++) {
      g_calibSamples[stepIndex][i] = accum[i] / rounds;
    }
  }

  g_calibStepReady[stepIndex] = true;
  g_calibCurrentStep = stepIndex + 1;

  const char *names[4] = {"Middle", "Right", "Left", "Middle (Forward)"};
  snprintf(g_calibStatusMsg, sizeof(g_calibStatusMsg), "Sample %d (%s) Recorded!", stepIndex + 1, names[stepIndex]);
  Serial.printf("[CALIB] Step %d (%s) recorded.\n", stepIndex + 1, names[stepIndex]);
}

bool computeCalibrationThresholds() {
  uint32_t sumThresholds = 0;
  int validCount = 0;

  for (int i = 0; i < LINE_SENSOR_COUNT; i++) {
    uint16_t sMin = 4095;
    uint16_t sMax = 0;

    for (int s = 0; s < CALIB_SAMPLE_COUNT; s++) {
      if (!g_calibStepReady[s]) continue;
      uint16_t val = g_calibSamples[s][i];
      if (val < sMin) sMin = val;
      if (val > sMax) sMax = val;
    }

    g_sensorMin[i] = sMin;
    g_sensorMax[i] = sMax;

    if (sMax > sMin + 200) { // Valid contrast detected
      g_sensorThresholds[i] = (sMin + sMax) / 2;
      sumThresholds += g_sensorThresholds[i];
      validCount++;
    } else {
      g_sensorThresholds[i] = g_pidConfig.threshold; // Fallback to global
    }
  }

  if (validCount > 0) {
    g_pidConfig.threshold = sumThresholds / validCount;
    snprintf(g_calibStatusMsg, sizeof(g_calibStatusMsg), "Calibrated! Avg Thr: %u (%d sensors)", g_pidConfig.threshold, validCount);
    g_calibCurrentStep = 5;
    Serial.printf("[CALIB] Calibration applied. New Global Thr: %u\n", g_pidConfig.threshold);
    return true;
  } else {
    snprintf(g_calibStatusMsg, sizeof(g_calibStatusMsg), "Warning: Low contrast between white/black");
    return false;
  }
}

void resetCalibration() {
  for (int s = 0; s < CALIB_SAMPLE_COUNT; s++) {
    g_calibStepReady[s] = false;
    for (int i = 0; i < LINE_SENSOR_COUNT; i++) {
      g_calibSamples[s][i] = 0;
    }
  }
  for (int i = 0; i < LINE_SENSOR_COUNT; i++) {
    g_sensorMin[i] = 4095;
    g_sensorMax[i] = 0;
    g_sensorThresholds[i] = 0;
  }
  g_calibCurrentStep = 0;
  snprintf(g_calibStatusMsg, sizeof(g_calibStatusMsg), "Ready: Place robot centered over line");
  Serial.println("[CALIB] Calibration samples reset.");
}

void startCalibrationServer() {
  if (g_calibServerActive) return;

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("Robotikka-Calib", "12345678");
  Serial.printf("\n[CALIB WEB] Access Point Started: 'Robotikka-Calib'\n");
  Serial.printf("[CALIB WEB] Web UI URL: http://%s/\n", WiFi.softAPIP().toString().c_str());

  // Also connect to station Wi-Fi if available
  if (ssid != nullptr && strlen(ssid) > 0) {
    WiFi.begin(ssid, password);
    Serial.printf("[CALIB WEB] Connecting to Station Wi-Fi: '%s'...\n", ssid);
  }

  s_server.on("/", HTTP_GET, handleRoot);
  s_server.on("/status", HTTP_GET, handleStatus);
  s_server.on("/setPID", HTTP_ANY, handleSetPID);
  s_server.on("/sample", HTTP_GET, handleSample);
  s_server.on("/toggleMoveForward", HTTP_GET, handleToggleMoveForward);
  s_server.on("/calculateThresholds", HTTP_GET, handleCalculateThresholds);
  s_server.on("/resetCalib", HTTP_GET, handleResetCalib);
  s_server.on("/exitCalib", HTTP_GET, handleExitCalib);

  s_server.begin();
  g_calibServerActive = true;
  snprintf(g_calibStatusMsg, sizeof(g_calibStatusMsg), "AP: 192.168.4.1 (Robotikka-Calib)");
}

void stopCalibrationServer() {
  if (!g_calibServerActive) return;

  s_server.close();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  g_calibServerActive = false;
  Serial.println("[CALIB WEB] Server stopped, Wi-Fi turned OFF");
}

void serviceCalibrationServer() {
  if (g_calibServerActive) {
    s_server.handleClient();
  }
}

