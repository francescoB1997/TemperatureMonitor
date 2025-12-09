String html = F(R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8"/>
<title>Camera di Lievitazione</title>
<style>
  body {
    background: #f7f2e8;
    font-family: Arial, sans-serif;
    margin: 0;
    padding: 0;
    text-align: center;
    color: #4a3c2d;
  }
  h1 {
    background: #d9a86c;
    padding: 15px;
    margin: 0;
    color: white;
    font-size: 26px;
    box-shadow: 0 2px 4px rgba(0,0,0,0.2);
  }
  .card {
    background: white;
    max-width: 400px;
    margin: 30px auto;
    padding: 20px;
    border-radius: 12px;
    box-shadow: 0 4px 8px rgba(0,0,0,0.15);
  }
  .label {
    margin-top: 20px;
    font-size: 16px;
    font-weight: bold;
  }
  .value {
    font-size: 32px;
    margin: 10px 0;
    color: #b05f3c;
  }
  input[type=range] {
    width: 90%;
    margin: 15px auto;
  }
  button {
    background: #d99058;
    color: white;
    padding: 10px 18px;
    border: none;
    border-radius: 8px;
    font-size: 16px;
    cursor: pointer;
    margin-top: 15px;
    transition: 0.2s;
  }
  button:hover {
    background: #c17843;
  }
</style>
</head>
<body>

<h1>Camera di Lievitazione</h1>

<div class="card">
  <div class="label">Temperatura Forno attuale</div>
  <div class="value">__TEMP_FORNO__</div>

  <div class="label">Temperatura Camera Filo attuale</div>
  <div class="value">__TEMP_FILO__</div>

  <div class="label">SetpointForno</div>
  <div id="lblForno" class="value">__SETPOINT_FORNO__ °C</div>

  <div class="label">SetpointCameraFilo</div>
  <div id="lblFilo" class="value">__SETPOINT_FILO__ °C</div>

  <form action="/setTempForno" method="POST">
    <input type="range" min="10" max="40" step="0.1" name="setPointForno" id="sliderForno" value="__SETPOINT_FORNO__">
    <button type="submit">Imposta</button>
  </form>

  <form action="/setTempFilo" method="POST">
    <input type="range" min="10" max="40" step="0.1" name="setPointFilo" id="sliderFilo" value="__SETPOINT_FILO__">
    <button type="submit">Imposta</button>
  </form>

  <div class="manual">
    <button id="btnFornoOn" class="button button-green">Accendi Forno</button>
    <button id="btnFornoOff" class="button button-red">Spegni Forno</button>
    <span id="relayBadgeForno" class="badge">__RELAY__</span>
    <button id="btnFiloOn" class="button button-green">Accendi Filo</button>
    <button id="btnFiloOff" class="button button-red">Spegni Filo</button>
    <span id="relayBadgeFilo" class="badge">__RELAY__</span>
  </div>

<script>
document.getElementById('btnFornoOn').addEventListener('click', function(){
  fetch('/relay_on?id=Forno').then(()=>updateBadge('Forno'));
});
document.getElementById('btnFornoOff').addEventListener('click', function(){
  fetch('/relay_off?id=Forno').then(()=>updateBadge('Forno'));
});
document.getElementById('btnFiloOn').addEventListener('click', function(){
  fetch('/relay_on?id=Filo').then(()=>updateBadge('Filo'));
});
document.getElementById('btnFiloOff').addEventListener('click', function(){
  fetch('/relay_off?id=Filo').then(()=>updateBadge('Filo'));
});

function updateBadge(id){
  fetch('/relay_state?id=' + id)
    .then(r => r.text())
    .then(text => {
    const badge = document.getElementById('relayBadge' + id);
    badge.innerText = text;
    badge.style.backgroundColor = (text === 'ON') ? '#28a745' : '#dc3545';
  });
}

// on load populate badge
updateBadge('Forno');
updateBadge('Filo');
</script>
</div>

<script>
  const sliderForno = document.getElementById('sliderForno');
  const sliderFilo  = document.getElementById('sliderFilo');
  const lblForno = document.getElementById('lblForno');
  const lblFilo = document.getElementById('lblFilo');

  sliderForno.oninput = function() {
    lblForno.innerText = this.value + " °C";
  }
  sliderFilo.oninput = function() {
    lblFilo.innerText = this.value + " °C";
  }
</script>

</body>
</html>
)rawliteral");