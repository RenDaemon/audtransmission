#include <WiFi.h>
#include <HTTPClient.h>
#include <SD.h>
#include <SPI.h>

const char* ssid = "IT 0529";
const char* password = "awikawok";
const char* serverUrl = "http://192.168.0.113:5000/upload";  // Replace with your actual IP

#define MIC_PIN 36  // MAX9814 analog output to GPIO36 (VP)

#define SD_CS 5  // SD card CS pin (adjust based on wiring)
#define SD_MISO 19  // GPIO19 (MISO)
#define SD_MOSI 23  // GPIO23 (MOSI)
#define SD_SCK  18  // GPIO18 (SCK)
#define SAMPLE_RATE 1600
#define BITS_PER_SAMPLE 8
#define RECORD_SECONDS 5
#define SAMPLE_COUNT (SAMPLE_RATE * RECORD_SECONDS)

uint8_t samples[SAMPLE_COUNT];
uint8_t wavHeader[44];

void writeWavHeader(uint8_t* header, int dataSize) {
  int byteRate = SAMPLE_RATE * BITS_PER_SAMPLE / 8;
  int blockAlign = BITS_PER_SAMPLE / 8;
  int chunkSize = 36 + dataSize;

  memcpy(header, "RIFF", 4);
  header[4] = chunkSize & 0xff;
  header[5] = (chunkSize >> 8) & 0xff;
  header[6] = (chunkSize >> 16) & 0xff;
  header[7] = (chunkSize >> 24) & 0xff;

  memcpy(header + 8, "WAVEfmt ", 8);
  header[16] = 16; header[17] = 0; header[18] = 0; header[19] = 0;
  header[20] = 1; header[21] = 0;
  header[22] = 1; header[23] = 0;
  header[24] = SAMPLE_RATE & 0xff;
  header[25] = (SAMPLE_RATE >> 8) & 0xff;
  header[26] = (SAMPLE_RATE >> 16) & 0xff;
  header[27] = (SAMPLE_RATE >> 24) & 0xff;
  header[28] = byteRate & 0xff;
  header[29] = (byteRate >> 8) & 0xff;
  header[30] = (byteRate >> 16) & 0xff;
  header[31] = (byteRate >> 24) & 0xff;
  header[32] = blockAlign;
  header[33] = 0;
  header[34] = BITS_PER_SAMPLE;
  header[35] = 0;
  memcpy(header + 36, "data", 4);
  header[40] = dataSize & 0xff;
  header[41] = (dataSize >> 8) & 0xff;
  header[42] = (dataSize >> 16) & 0xff;
  header[43] = (dataSize >> 24) & 0xff;
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected. IP address: " + WiFi.localIP().toString());

  if (!SD.begin(SD_CS)) {
    Serial.println("SD card initialization failed!");
    while (true);
  }
  Serial.println("SD card initialized.");
}

void loop() {
  Serial.println("Recording audio...");

  for (int i = 0; i < SAMPLE_COUNT; i++) {
    samples[i] = analogRead(MIC_PIN) >> 4;  // Convert 12-bit to 8-bit
    delayMicroseconds(625);                // 1600 Hz sampling rate
    yield();
  }

  writeWavHeader(wavHeader, SAMPLE_COUNT);

  if (!SD.exists("/aud")) {
    SD.mkdir("/aud");
  }

  String filename = "/aud/recording_" + String(millis()) + ".wav";
  File file = SD.open(filename, FILE_WRITE);
  if (file) {
    file.write(wavHeader, 44);
    file.write(samples, SAMPLE_COUNT);
    file.close();
    Serial.println("Saved to SD: " + filename);
  } else {
    Serial.println("Failed to write to SD.");
    return;
  }

  // Reopen for sending
  file = SD.open(filename);
  if (file && WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;

    http.begin(client, serverUrl);
    http.addHeader("Content-Type", "application/octet-stream");

    Serial.println("Uploading to server...");
    int httpCode = http.sendRequest("POST", &file, file.size());

    if (httpCode > 0) {
      Serial.printf("Upload complete. Server responded with: %d\n", httpCode);
    } else {
      Serial.printf("Upload failed. Error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
    file.close();
  } else {
    Serial.println("Failed to open file for sending or WiFi not connected.");
  }

  delay(10000);
}
