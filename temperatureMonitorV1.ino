#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ArduinoOTA.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// --------------------------------------------------------------------
// CONFIG
// --------------------------------------------------------------------
#define GPIO_SensoreForno 1
#define GPIO_Forno 2

#define GPIO_SensoreFilo 0
#define GPIO_Filo 4
Preferences prefs;


OneWire oneWireSensoreForno(GPIO_SensoreForno);
OneWire oneWireSensoreFilo(GPIO_SensoreFilo);

DallasTemperature sensoreForno(&oneWireSensoreForno);
DallasTemperature sensoreFilo(&oneWireSensoreFilo);

// Server + DNS
WebServer server(80);
DNSServer dnsServer;

float setPointForno = 26.0;
float setPointFilo = 26.0;
float lastSetPointForno = 0.0;
float lastSetPointFilo = 0.0;
float hysteresis = 3.0;
float globalTempForno = 100;
float globalTempFilo = 100;
unsigned long timeReadTemperature = 0;
uint8_t sampleCount = 0;
float sumSampleForno = 0;
float sumSampleFilo = 0;

String ssid = "";
String password = "";

// AP mode
const char* AP_SSID = "CameraLievitazione-1";
IPAddress apIP(192, 168, 4, 1);

bool autoMode = true;

// --------------------------------------------------------------------
// WEB HANDLERS
// --------------------------------------------------------------------

void handleCaptive() {
  String html = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1' />";
  html += "<title>Config WiFi</title></head><body>";
  html += "<h2>Configura WiFi</h2>";
  html += "<form action='/save' method='POST'>";
  html += "SSID: <input name='ssid'><br><br>";
  html += "Password: <input name='password'><br><br>";
  html += "<button type='submit'>Salva</button></form>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleSave() {
  if (server.hasArg("ssid") && server.hasArg("password")) {
    ssid = server.arg("ssid");
    password = server.arg("password");

    prefs.putString("ssid", ssid);
    prefs.putString("password", password);

    server.send(200, "text/html", "<h3>Salvato! Riavvio...</h3>");
    delay(1000);
    ESP.restart();
  } else {
    server.send(400, "text/plain", "Errore");
  }
}

// Pagina principale (STA)
void handleRoot() {
  //float t = 27.00;//tempSensor.readTemperature();

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
  <div class="value">__TEMP_FORNO__ °C</div>

  <div class="label">Temperatura Camera Filo attuale</div>
  <div class="value">__TEMP_FILO__ °C</div>

  <div class="label">SetpointForno</div>
  <div id="setLabel" class="value">__SETPOINT_FORNO__ °C</div>

  <div class="label">SetpointCameraFilo</div>
  <div id="setLabel" class="value">__SETPOINT_FILO__ °C</div>

  <form action="/setTempForno" method="POST">
    <input type="range" min="10" max="40" step="0.1" name="setPointForno" id="slider" value="__SETPOINT_FORNO__">
    <button type="submit">Imposta</button>
  </form>

  <form action="/setTempFilo" method="POST">
    <input type="range" min="10" max="40" step="0.1" name="setPointFilo" id="slider" value="__SETPOINT_FILO__">
    <button type="submit">Imposta</button>
  </form>

  <div class="manual">
    <button id="btnOn" class="button button-green">Accendi</button>
    <button id="btnOff" class="button button-red">Spegni</button>
    <span id="relayBadge" class="badge">__RELAY__</span>
  </div>

<script>
document.getElementById('btnOn').addEventListener('click', function(){
  fetch('/relay_on').then(()=>updateBadge());
});
document.getElementById('btnOff').addEventListener('click', function(){
  fetch('/relay_off').then(()=>updateBadge());
});

function updateBadge(){
  fetch('/relay_state').then(r => r.text()).then(text => {
    const badge = document.getElementById('relayBadge');
    badge.innerText = text;
    badge.style.backgroundColor = (text === 'ON') ? '#28a745' : '#dc3545';
  });
}

// on load populate badge
updateBadge();
</script>
</div>

<script>
  const slider = document.getElementById('slider');
  const setLabel = document.getElementById('setLabel');
  slider.oninput = function() {
    setLabel.innerText = this.value + " °C";
  }
</script>

</body>
</html>
)rawliteral");

  html.replace("__TEMP_FORNO__", String(globalTempForno));
  html.replace("__SETPOINT_FORNO__", String(setPointForno));
  html.replace("__TEMP_FILO__", String(globalTempForno));
  html.replace("__SETPOINT_FILO__", String(setPointForno));
  server.send(200, "text/html", html);
}

void handleSetpointForno() {
  if (server.hasArg("setPointForno")) {
    setPointForno = server.arg("setPointForno").toFloat();
    prefs.putFloat("lastSetPointForno", setPointForno);
  }
  else{
    Serial.println("No setPointForno in FORM");
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleSetpointFilo() {
  if (server.hasArg("setPointFilo")) {
    setPointFilo = server.arg("setPointFilo").toFloat();
    prefs.putFloat("lastSetPointFilo", setPointFilo);
  }
  else{
    Serial.println("No setPointFilo in FORM");
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleNotFound() {
  server.sendHeader("Location", "/", true);
  server.send(302);
}


// --------------------------------------------------------------------
// AP MODE (CAPTIVE PORTAL)
// --------------------------------------------------------------------
void startAPMode() {
  Serial.println("Avvio AP per configurazione...");

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

  // DNS: reindirizza tutto a 192.168.4.1
  dnsServer.start(53, "*", apIP);

  server.onNotFound(handleCaptive);
  server.on("/", handleCaptive);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();

  Serial.println("AP avviato!");
  Serial.println("IP: " + WiFi.softAPIP().toString());
}


// --------------------------------------------------------------------
// STA MODE
// --------------------------------------------------------------------
void startSTAMode() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  Serial.print("Connessione a: ");
  Serial.println(ssid);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 8000) {
    delay(200);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nConnessione fallita. Avvio AP...");
    startAPMode();
    return;
  }

  Serial.print("\nConnesso! IP: ");
  Serial.println(WiFi.localIP());

  ArduinoOTA.setHostname("lievitazione");
  ArduinoOTA.setPassword("1234");  // opzionale, ma consigliato

  ArduinoOTA
    .onStart([]() {
      Serial.println("OTA: Start");
    })
    .onEnd([]() {
      Serial.println("\nOTA: End");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress * 100) / total);
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]\n", error);
    });

  ArduinoOTA.begin();
  WiFi.setSleep(false);

  lastSetPointForno = prefs.getFloat("lastSetPoint", 27.5); // mettere default

  server.on("/", handleRoot);
  server.on("/setTempForno", HTTP_POST, handleSetpointForno);
  server.on("/setTempFilo", HTTP_POST, handleSetpointFilo);
  server.on("/relay_on", HTTP_GET, [](){
    digitalWrite(GPIO_Forno, HIGH);
    server.send(200, "text/plain", "OK");
  });

  server.on("/relay_off", HTTP_GET, [](){
    digitalWrite(GPIO_Forno, LOW);
    server.send(200, "text/plain", "OK");
  });

  // restituisce "ON" o "OFF"
  server.on("/relay_state", HTTP_GET, [](){
    String state = digitalRead(GPIO_Forno) ? "ON" : "OFF";
    server.send(200, "text/plain", state);
  });
  server.onNotFound(handleNotFound);
  server.begin();
}

void setup() {
  Serial.begin(115200);
  sensoreForno.begin();
  sensoreFilo.begin();
  
  pinMode(GPIO_Forno, OUTPUT);
  pinMode(GPIO_Filo, OUTPUT);
  digitalWrite(GPIO_Forno, LOW);  
  digitalWrite(GPIO_Filo, LOW);

  prefs.begin("wifi", false);
  ssid = prefs.getString("ssid", "");
  password = prefs.getString("password", "");

  if (ssid == "")
    startAPMode();
  else
    startSTAMode();
}

void loop() {
  if (WiFi.getMode() == WIFI_AP) {
    dnsServer.processNextRequest();
  }
  server.handleClient();
  ArduinoOTA.handle();

  if (WiFi.status() == WL_CONNECTED) {
    // controllo temperatura
    if (autoMode && (t < (setpoint - hysteresis))) {
      digitalWrite(GPIO_Forno, HIGH);
      digitalWrite(GPIO_Filo, HIGH);
    } else if (t >= setpoint) {
      digitalWrite(GPIO_Forno, LOW);
      digitalWrite(GPIO_Filo, LOW);
    }
  }

  if ( (millis() - timeReadTemperature) > 200)
  {
    // aggiorna la variabile globale t per aggiornare la temperatura
    sensoreForno.requestTemperatures(); 
    float temperaturaForno = sensoreForno.getTempCByIndex(0);

    sensoreFilo.requestTemperatures(); 
    float temperaturaFilo = sensoreFilo.getTempCByIndex(0);
    timeReadTemperature = millis();
  }
}
