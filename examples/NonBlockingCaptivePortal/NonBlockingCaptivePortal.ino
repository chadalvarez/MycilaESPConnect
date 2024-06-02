#include <ArduinoOTA.h>
#include <MycilaESPConnect.h>

AsyncWebServer server(80);
uint32_t lastLog = 0;
String hostname = "arduino-1";

void setup() {
  Serial.begin(115200);
  while (!Serial)
    continue;

  // serve your logo here
  server.on("/logo", HTTP_GET, [&](AsyncWebServerRequest* request) {
    // AsyncWebServerResponse* response = request->beginResponse_P(200, "image/png", _binary_data_logo_icon_png_gz_start, _binary_data_logo_icon_png_gz_end - _binary_data_logo_icon_png_gz_start);
    AsyncWebServerResponse* response = request->beginResponse_P(200, "image/png", "", 0);
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Cache-Control", "public, max-age=900");
    request->send(response);
  });

  // serve your home page here
  server.on("/home", HTTP_GET, [&](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", "Hello World!");
  });

#ifndef ESP8266
  // clear persisted config
  server.on("/clear", HTTP_GET, [&](AsyncWebServerRequest* request) {
    Serial.println("Clearing configuration...");
    ESPConnect.clearConfiguration();
    request->send(200);
  });
#endif

  server.on("/restart", HTTP_GET, [&](AsyncWebServerRequest* request) {
    Serial.println("Restarting...");
    request->send(200);
    ESP.restart();
  });

  // add a rewrite which is only applicable in AP mode and STA mode, but not in Captive Portal mode
  server.rewrite("/", "/home").setFilter([](__unused AsyncWebServerRequest* request) { return ESPConnect.getState() != ESPConnectState::PORTAL_STARTED; });

  // network state listener is required here in async mode
  ESPConnect.listen([](__unused ESPConnectState previous, ESPConnectState state) {
    JsonDocument doc;
    ESPConnect.toJson(doc.to<JsonObject>());
    serializeJson(doc, Serial);
    Serial.println();

    switch (state) {
      case ESPConnectState::NETWORK_CONNECTED:
      case ESPConnectState::AP_STARTED:
        server.begin();
        ArduinoOTA.setHostname(hostname.c_str());
#ifndef ESP8266
        ArduinoOTA.setMdnsEnabled(true);
#endif
        ArduinoOTA.begin();
        break;

      case ESPConnectState::NETWORK_DISCONNECTED:
        server.end();
        ArduinoOTA.end();
      default:
        break;
    }
  });

  ESPConnect.setAutoRestart(true);
  ESPConnect.setBlocking(false);
  ESPConnect.setCaptivePortalTimeout(180);
  ESPConnect.setConnectTimeout(15);

  Serial.println("====> Trying to connect to saved WiFi or will start portal in the background...");

#ifdef ESP8266
  ESPConnectConfig config = {.wifiSSID = "IoT", .wifiPassword = "", .apMode = false};
  ESPConnect.begin(server, hostname.c_str(), "Captive Portal SSID", "", config);
#else
  ESPConnect.begin(server, hostname.c_str(), "Captive Portal SSID");
#endif

  Serial.println("====> setup() completed...");
}

void loop() {
  ESPConnect.loop();
  ArduinoOTA.handle();
}
