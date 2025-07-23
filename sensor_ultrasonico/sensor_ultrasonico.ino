#include <WiFi.h>
#include <HTTPClient.h>

// Pines del sensor ultrasónico
#define TRIG_PIN 5
#define ECHO_PIN 18

// Red del Gateway (igual que el otro sensor)
const char* ssid = "RedSensorIoT";
const char* password = "sensorpass123";
const char* gatewayURL = "http://192.168.4.1/data";

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  WiFi.begin(ssid, password);
  Serial.print("Conectando al Gateway");

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nError al conectar. Reiniciando...");
    delay(2000);
    ESP.restart();
  }
  
  Serial.println("\nConectado. IP: " + WiFi.localIP().toString());
}

void loop() {
  static unsigned long lastSend = 0;
  if (millis() - lastSend > 5000) {  // Enviar cada 5 segundos
    lastSend = millis();

    float distance = medirDistancia();

    // Formato: distancia,checksum
    String baseData = "ULTRASONICO:" + String(distance, 2);

int checksum = 0;
for (unsigned int i = 0; i < baseData.length(); i++) {
  checksum += baseData[i];
}

String sensorData = baseData + "," + String(checksum);


    sendToGateway(sensorData);
  }
}

float medirDistancia() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);  // timeout 30 ms
  if (duration == 0) {
    return -1.0;  // Sin lectura válida
  }

  float distance = duration * 0.0343 / 2.0;
  Serial.print("Distancia: ");
  Serial.print(distance);
  Serial.println(" cm");

  return distance;
}

void sendToGateway(String data) {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    delay(2000);
    return;
  }

  HTTPClient http;
  http.begin(gatewayURL);
  http.addHeader("Content-Type", "text/plain");

  // Aquí ya no añades el prefijo, simplemente envías el mensaje completo.
  Serial.println("Enviando: " + data);

  int httpCode = http.POST(data);

  if (httpCode == HTTP_CODE_OK) {
    Serial.println("Envío exitoso");
  } else {
    Serial.println("Error HTTP: " + String(httpCode));
  }

  http.end();
}

