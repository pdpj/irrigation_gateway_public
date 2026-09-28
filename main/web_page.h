#pragma once

// Dashboard served on GET / — shows sensor readings (polling every 2s)
// and irrigation control. HTML/CSS/JS kept isolated here, separate from
// the firmware logic.
const char PAGE_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
  body{font-family:sans-serif; text-align:center; padding-top:30px; background:#f4f6f4; color:#1a241d;}
  h2{margin-bottom:4px;}
  .subtitle{color:#5a6b60; font-size:13px; margin-top:0; margin-bottom:24px;}
  .cards{display:flex; flex-wrap:wrap; justify-content:center; gap:14px; max-width:520px; margin:0 auto 28px;}
  .card{background:#fff; border:1px solid #d7e0d9; border-radius:10px; padding:14px 20px; min-width:120px;}
  .card .value{font-size:26px; font-weight:600; font-variant-numeric:tabular-nums;}
  .card .label{font-size:11px; text-transform:uppercase; letter-spacing:.04em; color:#5a6b60; margin-top:4px;}
  button{font-size:15px; padding:8px 18px; margin:0 6px; border-radius:6px; border:1px solid #cdd8cf; background:#fff; cursor:pointer;}
  button:hover{background:#eef2ee;}
  button:focus{outline:none;}
  button:disabled{color:#a9b3ac; cursor:default; background:#f4f6f4;}
  #lastUpdate{color:#5a6b60; font-size:13px; margin-top:10px; margin-bottom:16px;}
  hr{border:none; border-top:1px solid #d7e0d9; max-width:400px; margin:26px auto;}
  h3{font-size:14px; text-transform:uppercase; letter-spacing:.04em; color:#5a6b60; margin-bottom:14px; font-weight:600;}
</style>
</head>
<body>
  <h2>Irrigation Gateway</h2>

  <div class="cards">
    <div class="card">
      <div class="value" id="soilValue">--</div>
      <div class="label">Soil moisture</div>
    </div>
    <div class="card">
      <div class="value" id="tempValue">--</div>
      <div class="label">Air temperature</div>
    </div>
    <div class="card">
      <div class="value" id="airHumValue">--</div>
      <div class="label">Air humidity</div>
    </div>
  </div>

  <p id="lastUpdate">Last update: --</p>
  <button id="btnUpdate" onclick="refreshNow(this)">Update</button>

  <hr>

  <h3>Water control</h3>
  <button onclick="setIrrigationState(true, this)">Turn ON</button>
  <button onclick="setIrrigationState(false, this)">Turn OFF</button>
  <p id="result"></p>

  <script>
    function formatDateTime(date) {
      const dd = String(date.getDate()).padStart(2, '0');
      const mm = String(date.getMonth() + 1).padStart(2, '0');
      const yyyy = date.getFullYear();
      const hh = String(date.getHours()).padStart(2, '0');
      const mi = String(date.getMinutes()).padStart(2, '0');
      const ss = String(date.getSeconds()).padStart(2, '0');
      return hh + ':' + mi + ':' + ss + ' ' + dd + '/' + mm + '/' + yyyy;
    }

    async function setIrrigationState(state, button) {
      button.blur();
      button.disabled = true;               // locks the button as soon as it's clicked
      try {
        await fetch('/led', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ state: state })
        });
        // same margin used by Update - gives irrigationTask time to apply the pin
        setTimeout(async () => {
          await updateStatus();          // fetches the real state (may have already changed, e.g. timeout)
          button.disabled = false;       // returns the button to normal
        }, 300);
      } catch (error) {
        console.log('Failed to toggle water:', error);
        button.disabled = false;
      }
    }

    async function refreshNow(button) {
      button.blur();
      button.disabled = true;
      try {
        await fetch('/api/sensors/refresh', { method: 'POST' });
        // small margin so sensorsTask can wake up and sample before we re-query
        setTimeout(async () => {
          await updateStatus();
          button.disabled = false;
        }, 300);
      } catch (error) {
        console.log('Failed to request refresh:', error);
        button.disabled = false;
      }
    }

    async function updateStatus() {
      try {
        const response = await fetch('/api/status');
        const data = await response.json();
        document.getElementById('soilValue').textContent   = data.soilMoisturePercent.toFixed(1) + '%';
        document.getElementById('tempValue').textContent   = data.airTemperatureC.toFixed(1) + ' C';
        document.getElementById('airHumValue').textContent = data.airHumidityPercent.toFixed(1) + '%';

        // sensorAgeMs = how long ago (ms) this reading was sampled, relative
        // to when the ESP32 answered - we use this to compute the real
        // sampling time, not the time the browser received the response.
        const sampleTime = new Date(Date.now() - data.sensorAgeMs);
        document.getElementById('lastUpdate').textContent =
          'Last update: ' + formatDateTime(sampleTime);

        document.getElementById('result').textContent = 'WATER: ' + (data.ledState ? 'on' : 'off');
      } catch (error) {
        // ESP32 momentarily unresponsive (reset, WiFi reconnecting, etc.) - just retries next cycle
        console.log('Failed to fetch status:', error);
      }
    }

    updateStatus();
    setInterval(updateStatus, 2000);
  </script>
</body>
</html>
)HTML";
