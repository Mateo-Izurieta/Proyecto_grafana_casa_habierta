//RECEPTOR DE TODOS LOS PROYECTOS VIA WIFI,
//ENVIA LOS DATOS POR LORA

#include <WiFi.h>
#include <WebServer.h>
#include <RadioLib.h>

// Configuración WiFi AP
const char* apSSID = "RedSensorIoT";
const char* apPassword = "sensorpass123";
IPAddress localIP(192, 168, 4, 1);
IPAddress gatewayIP(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

// Configuración LoRa para Heltec V3
SX1262 lora = new Module(8, 14, 12, 13);
#define BAND 915.0
#define TX_POWER 22
#define SPREADING 10
#define CODING_RATE 7
#define SYNC_WORD 0xF3
#define BW 125.0

WebServer server(80);

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Iniciar AP WiFi
  WiFi.softAPConfig(localIP, gatewayIP, subnet);
  if (!WiFi.softAP(apSSID, apPassword)) {
    Serial.println("Error iniciando AP");
    while(1);
  }
  Serial.println("AP iniciado. IP: " + WiFi.softAPIP().toString());

  // Iniciar LoRa
  int state = lora.begin(BAND, BW, SPREADING, CODING_RATE, SYNC_WORD, TX_POWER);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("Error LoRa: ");
    Serial.println(state);
    while(1);
  }

  // Configuración avanzada
  lora.setRfSwitchPins(16, 17);
  lora.setCurrentLimit(140.0);
  
  Serial.println("Configuración LoRa lista:");
  Serial.println("Frecuencia: " + String(BAND) + " MHz");
  Serial.println("SF: SF" + String(SPREADING));

  // Configurar servidor web
  server.on("/data", HTTP_POST, handleData);
  server.begin();
  Serial.println("Servidor HTTP listo");
}

void handleData() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Método no permitido");
    return;
  }

  String data = server.arg("plain");
  Serial.println("Datos recibidos: " + data);

  // Verificar checksum
  int lastComma = data.lastIndexOf(',');
  if (lastComma == -1) {
    server.send(400, "text/plain", "Formato inválido");
    return;
  }

  String payload = data.substring(0, lastComma);
  int receivedChecksum = data.substring(lastComma+1).toInt();

  // Calcular checksum
  int calculatedChecksum = 0;
  for (unsigned int i = 0; i < payload.length(); i++) {
    calculatedChecksum += payload[i];
  }

  if (calculatedChecksum != receivedChecksum) {
    server.send(400, "text/plain", "Checksum inválido");
    return;
  }

  // Transmitir por LoRa con prefijo
  String loraMsg = "SENS:" + payload;
  int state = lora.transmit(loraMsg);
  
  if (state == RADIOLIB_ERR_NONE) {
    server.send(200, "text/plain", "OK");
    Serial.println("Enviado por LoRa: " + loraMsg);
  } else {
    server.send(500, "text/plain", "Error LoRa");
    Serial.print("Error transmisión: ");
    Serial.println(state);
  }
}

void loop() {
  server.handleClient();
}