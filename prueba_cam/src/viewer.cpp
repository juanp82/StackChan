#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <M5Cardputer.h>

static const char *ssid = "TimerCam-Prueba";
static const char *password = "Camara2026";
static bool paused = false;
static unsigned long retryAt = 0;
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
void setup() {
  auto cfg = M5.config(); M5Cardputer.begin(cfg, true);
  if (M5Cardputer.Display.width() < M5Cardputer.Display.height()) M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.setBrightness(180);
  status("Conectando...");
  WiFi.mode(WIFI_STA); WiFi.setSleep(false); WiFi.begin(ssid, password);
}
void loop() {
  M5Cardputer.update();
  if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
    auto keys = M5Cardputer.Keyboard.keysState();
    for (auto c : keys.word) if (c == 'p' || c == 'P' || c == ' ') paused = !paused;
  }
  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - retryAt > 5000) { status("Buscando camara"); WiFi.reconnect(); retryAt = millis(); }
    delay(50); return;
  }
  if (!paused && !showFrame()) { status("Sin imagen"); delay(500); }
  delay(30);
}
