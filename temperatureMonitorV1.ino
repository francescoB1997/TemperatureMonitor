#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "index_html.h"
// --------------------------------------------------------------------
// CONFIG
// --------------------------------------------------------------------
#define GPIO_SensoreForno 1
#define GPIO_Forno 2

#define GPIO_SensoreFilo 0
#define GPIO_Filo 4
Preferences wifiPrefs;
Preferences dataPrefs;



OneWire oneWireSensoreForno(GPIO_SensoreForno);
OneWire oneWireSensoreFilo(GPIO_SensoreFilo);

DallasTemperature sensoreForno(&oneWireSensoreForno);
DallasTemperature sensoreFilo(&oneWireSensoreFilo);

// Server + DNS
WebServer server(80);
DNSServer dnsServer;

float setPointForno = 26.0;
float setPointFilo = 26.0;
float hysteresis = 1.2;
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

    wifiPrefs.putString("ssid", ssid);
    wifiPrefs.putString("password", password);

    server.send(200, "text/html", "<h3>Salvato! Riavvio...</h3>");
    wifiPrefs.end();
    delay(1000);
    ESP.restart();
  } else {
    server.send(400, "text/plain", "Errore");
  }

}

// Pagina principale (STA)
void handleRoot() {
  //float t = 27.00;//tempSensor.readTemperature();



  html.replace("__TEMP_FORNO__", (globalTempForno < -20) ? "NON CONNESSO" : String(globalTempForno) + " °C");
  html.replace("__SETPOINT_FORNO__", String(setPointForno));
  html.replace("__TEMP_FILO__", (globalTempFilo < -20) ? "NON CONNESSO" : String(globalTempFilo)+ " °C");
  html.replace("__SETPOINT_FILO__", String(setPointFilo));
  server.send(200, "text/html", html);
}

void handleSetpointForno() {
  if (server.hasArg("setPointForno")) {
    setPointForno = server.arg("setPointForno").toFloat();
    dataPrefs.putFloat("sPForno", setPointForno);
  }
  // else
  //  Serial.println("No setPointForno in FORM");
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleSetpointFilo() {
  if (server.hasArg("setPointFilo")) {
    setPointFilo = server.arg("setPointFilo").toFloat();
    dataPrefs.putFloat("sPFilo", setPointFilo);
  }
  //else
  //  Serial.println("No setPointFilo in FORM");
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
  //Serial.println("Avvio AP per configurazione...");

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
  wifiPrefs.end();

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

  setPointForno = dataPrefs.getFloat("sPForno", 27.5); // mettere default
  setPointFilo = dataPrefs.getFloat("sPFilo", 27.5); // mettere default

  server.on("/", handleRoot);
  server.on("/setTempForno", HTTP_POST, handleSetpointForno);
  server.on("/setTempFilo", HTTP_POST, handleSetpointFilo);
  server.on("/relay_on", HTTP_GET, [](){
    String id = "";
    if (server.hasArg("id")) {
      id = server.arg("id");
      uint8_t gpioTemp = (id == "Filo") ? GPIO_Filo  : GPIO_Forno;
      digitalWrite(gpioTemp, HIGH);
    server.send(200, "text/plain", "OK");
      return;
    }
    server.send(400, "text/plain", "Manca l'id");
  });

  server.on("/relay_off", HTTP_GET, [](){
    String id = "";
    if (server.hasArg("id")) {
      id = server.arg("id");
      uint8_t gpioTemp = (id == "Filo") ? GPIO_Filo  : GPIO_Forno;
      digitalWrite(gpioTemp, LOW);
    server.send(200, "text/plain", "OK");
      return;
    }
    server.send(400, "text/plain", "Manca l'id");
  });

  // restituisce "ON" o "OFF"
  server.on("/relay_state", HTTP_GET, [](){
    String id = "";
    if (server.hasArg("id")) {
        id = server.arg("id");
        uint8_t gpio_temp = (id == "Filo") ? GPIO_Filo  : GPIO_Forno;
        String state = digitalRead(gpio_temp) ? "ON" : "OFF";
    server.send(200, "text/plain", state);
        return;
    }
    server.send(400, "text/plain", "Manca l'id");
  });
  server.onNotFound(handleNotFound);
  server.begin();
}

void setup() {
  MDNS.begin("esp32c3");
  //Serial.begin(115200);
  sensoreForno.begin();
  sensoreFilo.begin();
  
  pinMode(GPIO_Forno, OUTPUT);
  pinMode(GPIO_Filo, OUTPUT);
  digitalWrite(GPIO_Forno, LOW);  
  digitalWrite(GPIO_Filo, LOW);

  wifiPrefs.begin("wifi", false);
  dataPrefs.begin("data", false);

  ssid = wifiPrefs.getString("ssid", "");
  password = wifiPrefs.getString("password", "");

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
    if (autoMode && ( globalTempForno < (setPointForno - hysteresis))) {
      digitalWrite(GPIO_Forno, HIGH);
    } else if (globalTempForno >= setPointForno) {
      digitalWrite(GPIO_Forno, LOW);
    }
    if (autoMode && ( globalTempFilo < (setPointFilo - hysteresis))) {
      digitalWrite(GPIO_Filo, HIGH);
    } else if (globalTempFilo >= setPointFilo) {
      digitalWrite(GPIO_Filo, LOW);
    }
  }

  if ( (millis() - timeReadTemperature) > 200)
  {
    // aggiorna la variabile globale t per aggiornare la temperatura
    sensoreForno.requestTemperatures(); 
    //float temperaturaForno = sensoreForno.getTempCByIndex(0);
    globalTempForno = sensoreForno.getTempCByIndex(0);
    if (globalTempForno < -20)
      sensoreForno.begin();
    sensoreFilo.requestTemperatures(); 
    globalTempFilo = sensoreFilo.getTempCByIndex(0);
    if (globalTempFilo < -20)
      sensoreFilo.begin();
    //float temperaturaFilo = sensoreFilo.getTempCByIndex(0);
    timeReadTemperature = millis();
  }
}
