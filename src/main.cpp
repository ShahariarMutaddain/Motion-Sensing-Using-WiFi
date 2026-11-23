#include <Arduino.h>
#include <WiFi.h>

namespace {

constexpr char TARGET_SSID[] = "Area 51";
constexpr uint8_t ACTIVITY_WINDOW_SAMPLES = 5;
constexpr uint8_t REQUIRED_CHANGE_SAMPLES = 3;
constexpr int SIGNAL_ACTIVITY_THRESHOLD_DB = 5;
constexpr int SIGNAL_QUIET_THRESHOLD_DB = 2;
constexpr uint32_t SCAN_MAX_MS_PER_CHANNEL = 100;
constexpr uint32_t SAMPLE_INTERVAL_MS = 250;

String targetBssid;
String targetSsid;
int targetChannel = 0;
int rssiWindow[ACTIVITY_WINDOW_SAMPLES] = {};
uint8_t windowCount = 0;
uint8_t nextWindowIndex = 0;
uint8_t changeCount = 0;
uint8_t clearCount = 0;
bool changeDetected = false;

void listVisibleNetworks() {
  Serial.println("Scanning for visible Wi-Fi networks...");
  const int networkCount =
      WiFi.scanNetworks(false, false, false, SCAN_MAX_MS_PER_CHANNEL);
  if (networkCount < 0) {
    Serial.println("Wi-Fi scan failed.");
    WiFi.scanDelete();
    return;
  }

  if (networkCount == 0) {
    Serial.println("No Wi-Fi networks found.");
  } else {
    Serial.printf("Found %d network(s):\n", networkCount);
    for (int i = 0; i < networkCount; ++i) {
      Serial.printf("%d: %s | %ld dBm | channel %ld\n",
                    i + 1, WiFi.SSID(i).c_str(),
                    static_cast<long>(WiFi.RSSI(i)),
                    static_cast<long>(WiFi.channel(i)));
    }
  }

  WiFi.scanDelete();
}

void resetSignalHistory() {
  windowCount = 0;
  nextWindowIndex = 0;
  changeCount = 0;
  clearCount = 0;
  changeDetected = false;
  Serial.println("Signal history reset. Monitoring short-term Wi-Fi signal variation.");
}

bool readTargetRssi(int &rssi) {
  const char *ssidFilter = TARGET_SSID[0] == '\0' ? nullptr : TARGET_SSID;
  const int networkCount =
      WiFi.scanNetworks(false, false, false, SCAN_MAX_MS_PER_CHANNEL,
                        static_cast<uint8_t>(targetChannel), ssidFilter);
  if (networkCount < 0) {
    Serial.println("Wi-Fi scan failed.");
    WiFi.scanDelete();
    return false;
  }

  int selectedNetwork = -1;
  int strongestRssi = -127;

  for (int i = 0; i < networkCount; ++i) {
    const String ssid = WiFi.SSID(i);
    const String bssid = WiFi.BSSIDstr(i);

    if (targetBssid.length() > 0) {
      if (!bssid.equalsIgnoreCase(targetBssid)) {
        continue;
      }
    } else if (TARGET_SSID[0] != '\0') {
      if (ssid != TARGET_SSID) {
        continue;
      }
    }

    const int candidateRssi = WiFi.RSSI(i);
    if (selectedNetwork < 0 || candidateRssi > strongestRssi) {
      selectedNetwork = i;
      strongestRssi = candidateRssi;
    }
  }

  if (selectedNetwork < 0) {
    WiFi.scanDelete();
    return false;
  }

  if (targetBssid.length() == 0) {
    targetSsid = WiFi.SSID(selectedNetwork);
    targetBssid = WiFi.BSSIDstr(selectedNetwork);
    targetChannel = WiFi.channel(selectedNetwork);
    Serial.printf("Monitoring SSID \"%s\", BSSID %s, channel %d\n",
                  targetSsid.c_str(), targetBssid.c_str(), targetChannel);
  }

  rssi = WiFi.RSSI(selectedNetwork);
  WiFi.scanDelete();
  return true;
}

void processRssi(int rssi) {
  rssiWindow[nextWindowIndex] = rssi;
  nextWindowIndex = (nextWindowIndex + 1) % ACTIVITY_WINDOW_SAMPLES;
  if (windowCount < ACTIVITY_WINDOW_SAMPLES) {
    ++windowCount;
  }

  if (windowCount < ACTIVITY_WINDOW_SAMPLES) {
    Serial.printf("Collecting signal samples %u/%u: %d dBm\n",
                  windowCount, ACTIVITY_WINDOW_SAMPLES, rssi);
    return;
  }

  int minimumRssi = rssiWindow[0];
  int maximumRssi = rssiWindow[0];
  for (uint8_t i = 1; i < ACTIVITY_WINDOW_SAMPLES; ++i) {
    minimumRssi = min(minimumRssi, rssiWindow[i]);
    maximumRssi = max(maximumRssi, rssiWindow[i]);
  }

  const int signalRange = maximumRssi - minimumRssi;
  Serial.printf("RSSI %d dBm | recent range %d dB | ",
                rssi, signalRange);

  if (!changeDetected) {
    if (signalRange >= SIGNAL_ACTIVITY_THRESHOLD_DB) {
      if (++changeCount >= REQUIRED_CHANGE_SAMPLES) {
        changeDetected = true;
        changeCount = 0;
        Serial.println("SIGNAL ACTIVITY detected (not a confirmed person/movement)");
        return;
      }
    } else {
      changeCount = 0;
    }
    Serial.println("no significant signal variation");
    return;
  }

  changeCount = 0;
  if (signalRange <= SIGNAL_QUIET_THRESHOLD_DB) {
    if (++clearCount >= REQUIRED_CHANGE_SAMPLES) {
      changeDetected = false;
      clearCount = 0;
      Serial.println("signal is stable again");
      return;
    }
  } else {
    clearCount = 0;
  }
  Serial.println("signal variation continues");
}

}  // namespace

void setup() {
  Serial.begin(115200);

  const uint32_t serialStart = millis();
  while (!Serial && millis() - serialStart < 3000) {
    delay(10);
  }
  delay(100);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.println("ESP32-S3 Wi-Fi signal-change monitor");
  Serial.println("This measures RSSI variation; it cannot confirm or identify a person.");
  Serial.println("Monitoring SSID: \"Area 51\"");
  Serial.println("Send 'l' over Serial to list nearby networks.");
  Serial.println("Send 'c' over Serial to reset the signal history.");
  listVisibleNetworks();
  resetSignalHistory();
}

void loop() {
  while (Serial.available() > 0) {
    const char command = static_cast<char>(Serial.read());
    if (command == 'c' || command == 'C') {
      resetSignalHistory();
    } else if (command == 'l' || command == 'L') {
      listVisibleNetworks();
    }
  }

  int rssi = 0;
  if (readTargetRssi(rssi)) {
    processRssi(rssi);
  } else if (targetBssid.length() == 0) {
    Serial.println("Configured SSID not found; retrying.");
  } else {
    Serial.printf("Target access point \"%s\" not found; retrying.\n",
                  targetSsid.c_str());
  }

  delay(SAMPLE_INTERVAL_MS);
}
