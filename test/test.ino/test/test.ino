#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>

const char* ssid = "Ksabar";
const char* password = "Pr!vat3N3t"; // Empty for open WiFi
const char* serverUrl = "http://192.168.186.208:5000/upload"; // Replace with your WSL IP

const int sampleRate = 1600;         // 1600 Hz sampling rate
const int bitsPerSample = 8;         // 8-bit samples (1 byte each)
const int recordSeconds = 5;
const int sampleCount = sampleRate * recordSeconds;

uint8_t samples[sampleCount];
uint8_t wavHeader[44];               // Standard WAV header is 44 bytes

void writeWavHeader(uint8_t* header, int dataSize) {
  int byteRate = sampleRate * bitsPerSample / 8;
  int blockAlign = bitsPerSample / 8;
  int chunkSize = 36 + dataSize;

  memcpy(header, "RIFF", 4);
  header[4] = (chunkSize) & 0xff;
  header[5] = (chunkSize >> 8) & 0xff;
  header[6] = (chunkSize >> 16) & 0xff;
  header[7] = (chunkSize >> 24) & 0xff;

  memcpy(header + 8, "WAVEfmt ", 8);
  header[16] = 16; header[17] = 0; header[18] = 0; header[19] = 0;
  header[20] = 1; header[21] = 0;
  header[22] = 1; header[23] = 0;
  header[24] = sampleRate & 0xff;
  header[25] = (sampleRate >> 8) & 0xff;
  header[26] = (sampleRate >> 16) & 0xff;
  header[27] = (sampleRate >> 24) & 0xff;
  header[28] = byteRate & 0xff;
  header[29] = (byteRate >> 8) & 0xff;
  header[30] = (byteRate >> 16) & 0xff;
  header[31] = (byteRate >> 24) & 0xff;
  header[32] = blockAlign;
  header[33] = 0;
  header[34] = bitsPerSample;
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
  Serial.println("\nWiFi connected.");
}

void loop() {
  Serial.println("Recording 5 seconds...");
  for (int i = 0; i < sampleCount; i++) {
    samples[i] = analogRead(A0) >> 2;  // 10-bit to 8-bit
    delayMicroseconds(625);           // 1600 Hz
    yield();
  }

  Serial.println("Preparing WAV file...");
  writeWavHeader(wavHeader, sampleCount);

  const int totalSize = 44 + sampleCount;
  uint8_t* wavData = new uint8_t[totalSize];
  memcpy(wavData, wavHeader, 44);
  memcpy(wavData + 44, samples, sampleCount);

  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;

    http.begin(client, serverUrl);
    http.addHeader("Content-Type", "application/octet-stream");

    Serial.println("Uploading to server...");
    int httpCode = http.sendRequest("POST", wavData, totalSize);

    if (httpCode > 0) {
      Serial.printf("Upload complete. Server response: %d\n", httpCode);
    } else {
      Serial.printf("Upload failed. Error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  } else {
    Serial.println("WiFi not connected.");
  }

  delete[] wavData;

  delay(10000);  // Wait 10 seconds before next loop
}
