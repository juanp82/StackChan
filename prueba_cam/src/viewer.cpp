#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <M5Cardputer.h>

static const char *ssid = "TimerCam-Prueba";
static const char *password = "Camara2026";
static bool paused = false;
static unsigned long retryAt = 0;
static unsigned long confirmUntil = 0;
static bool cameraOff = false;
void status(const char *message) {
  M5Cardputer.Display.fillScreen(TFT_BLACK);
  M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5Cardputer.Display.setTextSize(2);
  M5Cardputer.Display.setCursor(5, 40);
  M5Cardputer.Display.println(message);
}
bool showFrame() {
  WiFiClient client; HTTPClient http;
  http.setTimeout(3000);
  if (!http.begin(client, "http://192.168.4.1/jpg")) return false;
  if (http.GET() != HTTP_CODE_OK) { http.end(); return false; }
  int length = http.getSize();
  if (length <= 0 || length > 70000) { http.end(); return false; }
  uint8_t *buf = (uint8_t*)malloc(length);
  if (!buf) { http.end(); return false; }
  size_t got = http.getStream().readBytes(buf, length);
  bool ok = got == (size_t)length && buf[0] == 0xff && buf[1] == 0xd8;
  if (ok) M5Cardputer.Display.drawJpg(buf, length, 40, 7);
  free(buf); http.end(); return ok;
}
void shutdownCamera() {
  WiFiClient client; HTTPClient http;
  http.setTimeout(3000);
  if (!http.begin(client, "http://192.168.4.1/off")) { status("Error conexion"); return; }
  int result = http.POST("");
  http.end();
  if (result == 200) {
    cameraOff = true;
    WiFi.disconnect();
    status("Camara apagada");
  } else { status("No se apago"); delay(1500); }
}
void setup() {
  auto cfg = M5.config(); M5Cardputer.begin(cfg, true);
  if (M5Cardputer.Display.width() < M5Cardputer.Display.height()) M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.setBrightness(180);
  status("P: pausa  A: off");
  delay(2000);
  status("Conectando...");
  WiFi.mode(WIFI_STA); WiFi.setSleep(false); WiFi.begin(ssid, password);
}
void loop() {
  M5Cardputer.update();
  if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
    auto keys = M5Cardputer.Keyboard.keysState();
    for (auto c : keys.word) {
      if (c == 'a' || c == 'A') {
        if (cameraOff) break;
        if (confirmUntil && (long)(confirmUntil - millis()) > 0) { confirmUntil = 0; shutdownCamera(); }
        else if (WiFi.status() == WL_CONNECTED) { confirmUntil = millis() + 5000; status("A otra vez: OFF"); }
      }
      if (c == 'p' || c == 'P' || c == ' ') { paused = !paused; confirmUntil = 0; }
    }
  }
  if (cameraOff) { delay(100); return; }
  if (confirmUntil && (long)(confirmUntil - millis()) > 0) { delay(30); return; }
  confirmUntil = 0;
  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - retryAt > 5000) { status("Buscando camara"); WiFi.reconnect(); retryAt = millis(); }
    delay(50); return;
  }
  if (!paused && !showFrame()) { status("Sin imagen"); delay(500); }
  delay(30);
}
