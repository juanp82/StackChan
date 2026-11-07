#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "esp_camera.h"

WebServer server(80);
void snapshot() {
  camera_fb_t *frame = esp_camera_fb_get();
  if (!frame) { server.send(503, "text/plain", "Camera error"); return; }
  server.setContentLength(frame->len);
  server.send(200, "image/jpeg", "");
  WiFiClient client = server.client();
  size_t sent = 0;
  while (sent < frame->len && client.connected()) {
    size_t n = client.write(frame->buf + sent, frame->len - sent);
    if (!n) break;
    sent += n;
  }
  esp_camera_fb_return(frame);
}
void setup() {
  pinMode(33, OUTPUT); digitalWrite(33, HIGH); // TimerCam power hold
  Serial.begin(115200);
  camera_config_t c = {};
  c.pin_pwdn = -1; c.pin_reset = 15; c.pin_xclk = 27;
  c.pin_sccb_sda = 25; c.pin_sccb_scl = 23;
  c.pin_d7 = 19; c.pin_d6 = 36; c.pin_d5 = 18; c.pin_d4 = 39;
  c.pin_d3 = 5; c.pin_d2 = 34; c.pin_d1 = 35; c.pin_d0 = 32;
  c.pin_vsync = 22; c.pin_href = 26; c.pin_pclk = 21;
  c.xclk_freq_hz = 20000000; c.ledc_timer = LEDC_TIMER_0; c.ledc_channel = LEDC_CHANNEL_0;
  c.pixel_format = PIXFORMAT_JPEG; c.frame_size = FRAMESIZE_QQVGA;
  c.jpeg_quality = 15; c.fb_count = 1; c.fb_location = CAMERA_FB_IN_PSRAM;
  if (esp_camera_init(&c) != ESP_OK) { Serial.println("Camera init failed"); return; }
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP("TimerCam-Prueba", "Camara2026")) { Serial.println("AP failed"); return; }
  server.on("/jpg", HTTP_GET, snapshot);
  server.on("/", HTTP_GET, [](){ server.send(200, "text/html", "<h2>TimerCam</h2><img src='/jpg'>"); });
  server.begin();
  Serial.println(WiFi.softAPIP());
}
void loop() { server.handleClient(); delay(1); }
