#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme;

const char* ssid = "RedSensorIoT";
const char* password = "sensorpass123";
const char* gatewayURL = "http://192.168.4.1/data";

String sharedData = "";
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

// Tarea en núcleo 0: leer datos del sensor
void taskReadSensor(void *parameter) {
  while (true) {
    String data = String(bme.readTemperature(), 1) + "," +
                  String(bme.readHumidity(), 1) + "," +
                  String(bme.readPressure() / 100.0F, 1) + "," +
                  String(bme.readAltitude(SEALEVELPRESSURE_HPA), 1);

    int checksum = 0;
    for (unsigned int i = 0; i < data.length(); i++) {
      checksum += data[i];
    }
    data += "," + String(checksum);

    // Imprimir datos recopilados
    Serial.println("[Sensor] Datos recopilados: " + data);

    portENTER_CRITICAL(&mux);
    sharedData = data;
    portEXIT_CRITICAL(&mux);

    vTaskDelay(10000 / portTICK_PERIOD_MS);  // Esperar 10 segundos
  }
}

// Tarea en núcleo 1: enviar los datos
void taskSendData(void *parameter) {
  while (true) {
    if (WiFi.status() == WL_CONNECTED) {
      String dataToSend = "";

      portENTER_CRITICAL(&mux);
      dataToSend = sharedData;
      portEXIT_CRITICAL(&mux);

      if (dataToSend != "") {
        HTTPClient http;
        http.begin(gatewayURL);
        http.addHeader("Content-Type", "text/plain");

        Serial.println("[Envío] Enviando datos al Gateway: " + dataToSend);
        int httpCode = http.POST(dataToSend);

        if (httpCode == HTTP_CODE_OK) {
          Serial.println("[Envío] Envío exitoso");
        } else {
          Serial.println("[Envío] Error HTTP: " + String(httpCode));
        }
        http.end();
      }
    } else {
      Serial.println("[WiFi] Desconectado. Intentando reconectar...");
      WiFi.reconnect();
      delay(2000);
    }

    vTaskDelay(10000 / portTICK_PERIOD_MS);  // Esperar 10 segundos
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  if (!bme.begin(0x76)) {
    Serial.println("[Error] No se detectó el BME280");
    while (1);
  }

  WiFi.begin(ssid, password);
  Serial.print("[WiFi] Conectando");

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\n[WiFi] Error al conectar. Reiniciando...");
    delay(2000);
    ESP.restart();
  }

  Serial.println("\n[WiFi] Conectado. IP: " + WiFi.localIP().toString());

  // Crear tareas en núcleos distintos
  xTaskCreatePinnedToCore(taskReadSensor, "LecturaSensor", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(taskSendData, "EnvioDatos", 4096, NULL, 1, NULL, 1);
}

void loop() {
  // El loop no hace nada, ya que todo corre en tareas
}
