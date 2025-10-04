#include <WiFi.h>

const char* WIFI_SSID = "BAJA AP";
const char* WIFI_PASS = "password123";

const char* SERVER_IP = "192.168.0.104"; // <-- replace with your laptop's LAN IP
const uint16_t SERVER_PORT = 5055;

WiFiClient client;

uint32_t intervalMs = 500;  // default 1s
uint32_t lastSend = 0;
uint32_t counter = 0;

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.printf("[wifi] Connecting to \"%s\"...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[wifi] Connected. IP: ");
    Serial.println(WiFi.localIP());
    // Optional: sometimes helps in noisy environments
    // WiFi.setSleep(false);
  } else {
    Serial.println("[wifi] Failed to connect.");
  }
}

bool connectServer() {
  if (client.connected()) return true;

  Serial.printf("[tcp] Connecting to %s:%u ...\n", SERVER_IP, SERVER_PORT);
  if (client.connect(SERVER_IP, SERVER_PORT)) {
    client.setTimeout(1000);
    Serial.println("[tcp] Connected to server.");
    // Say hello
    client.print("HELLO ESP32\n");
    return true;
  }
  Serial.println("[tcp] Connect failed.");
  return false;
}

void sendSample() {
  // Generate a random value. You can swap in sensor data later.
  // Use esp_random() for a 32-bit random number (returns uint32_t).
  uint32_t r = esp_random() & 0xFFFF; // 0..65535
  uint32_t now = millis();

  client.printf("{\"uptime\":%lu,\"count\":%lu,\"accel_x\":%f,\"accel_y\":%f,\"accel_z\":%f}\n",
                  (unsigned long)now,
                  (unsigned long)counter,
                  0.0000000, 1.11111111, 2.2222222);
  counter++;
}




void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();

  connectWiFi();
  connectServer();
  lastSend = millis();
}

void loop() {
  // Keep Wi-Fi and server connection healthy
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
    delay(200);
  }
  if (!client.connected()) {
    static uint32_t lastTry = 0;
    uint32_t now = millis();
    if (now - lastTry > 3000) {
      lastTry = now;
      connectServer();
    }
    delay(5);
    return;
  }

  // Process incoming commands from the laptop
  //readCommands();

  // Periodic send (unless paused)
  uint32_t now = millis();
  if (now - lastSend >= intervalMs) {
    lastSend = now;
    if (client.connected()) {
      
        sendSample();
      
    }
  }
}
