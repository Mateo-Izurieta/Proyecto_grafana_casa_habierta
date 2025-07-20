//SENSOR PROTOTIPO PARA PROYECTO, MEDIDOR DE TEMP-ALT-HUM-PRES
//ENVIA LOS DATOS POR WIFI AL LORA ENCARGADO DE RECOGER LOS DATOS
//DE PROYECTOS
#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme;

const char* ssid = "RedSensorIoT";
const char* password = "sensorpass123";
const char* gatewayURL = "http://192.168.4.1/data";

void setup() {
  Serial.begin(115200);
  delay(1000);

  if (!bme.begin(0x76)) {
    Serial.println("¡Error al iniciar BME280!");
    while(1);
  }

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
  if (millis() - lastSend > 10000) { // Enviar cada 10 segundos
    lastSend = millis();
    
    // Formato: temperatura,humedad,presion,altitud,checksum
    String sensorData = String(bme.readTemperature(), 1) + "," +
                       String(bme.readHumidity(), 1) + "," +
                       String(bme.readPressure() / 100.0F, 1) + "," +
                       String(bme.readAltitude(SEALEVELPRESSURE_HPA), 1);
    
    // Calcular checksum
    int checksum = 0;
    for (unsigned int i = 0; i < sensorData.length(); i++) {
      checksum += sensorData[i];
    }
    sensorData += "," + String(checksum);

    sendToGateway(sensorData);
  }
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

  Serial.println("Enviando: " + data);
  int httpCode = http.POST(data);
  
  if (httpCode == HTTP_CODE_OK) {
    Serial.println("Envío exitoso");
  } else {
    Serial.println("Error HTTP: " + String(httpCode));
  }
  
  http.end();
}