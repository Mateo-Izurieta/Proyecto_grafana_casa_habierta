//RECEPTOR DE TODOS LOS DATOS POR LORA, Y LOS ENVIA A LA PC PARA
//MOSTRARLOS POR GRAFANA
#include <WiFi.h>
#include <HTTPClient.h>
#include <RadioLib.h>

// Configuración WiFi (usar tus credenciales)
const char* wifiSSID = "wifiName";
const char* wifiPassword = "wifiPassword";

// Configuración InfluxDB
const char* influxServer = "url influxdb";
const char* influxToken = "token influxdb";

// Configuración LoRa (debe coincidir con Gateway)
SX1262 lora = new Module(8, 14, 12, 13);
#define BAND 915.0
#define SPREADING 10
#define SYNC_WORD 0xF3

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Iniciar LoRa
  int state = lora.begin(BAND, 125.0, SPREADING, 7, SYNC_WORD, 17);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("Error LoRa: ");
    Serial.println(state);
    while(1);
  }

  // Configuración avanzada
  lora.setRfSwitchPins(16, 17);
  
  Serial.println("Receptor LoRa iniciado. Esperando datos...");

  // Conectar WiFi
  WiFi.begin(wifiSSID, wifiPassword);
  Serial.print("Conectando a WiFi");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 15) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConectado a WiFi");
  } else {
    Serial.println("\nError WiFi. Operando offline.");
  }
}

void loop() {
  String receivedData;
  int state = lora.receive(receivedData);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("\nDatos recibidos: " + receivedData);
    
    // Verificar formato "SENS:temp,hum,pres,alt"
    if (receivedData.startsWith("SENS:")) {
  String sensorData = receivedData.substring(5);
  processAndSendData(sensorData);
} else if (receivedData.startsWith("ULTRASONICO:")) {
  String distData = receivedData.substring(12);
  processAndSendUltrasonico(distData);
}

    
    // Mostrar calidad de señal
    Serial.print("RSSI: ");
    Serial.print(lora.getRSSI());
    Serial.println(" dBm");
  }
  else if (state != RADIOLIB_ERR_RX_TIMEOUT) {
    Serial.print("Error recepción: ");
    Serial.println(state);
  }
}

void processAndSendData(String data) {
  // Parsear datos (temp,hum,pres,alt)
  int commas[3];
  commas[0] = data.indexOf(',');
  commas[1] = data.indexOf(',', commas[0]+1);
  commas[2] = data.indexOf(',', commas[1]+1);

  if (commas[0] == -1 || commas[1] == -1 || commas[2] == -1) {
    Serial.println("Formato de datos inválido");
    return;
  }

  String temp = data.substring(0, commas[0]);
  String hum = data.substring(commas[0]+1, commas[1]);
  String pres = data.substring(commas[1]+1, commas[2]);
  String alt = data.substring(commas[2]+1);

  // Crear payload para InfluxDB
  String payload = "sensores,ubicacion=habitacion ";
  payload += "temperatura=" + temp + ",";
  payload += "humedad=" + hum + ",";
  payload += "presion=" + pres + ",";
  payload += "altura=" + alt;

  // Enviar a InfluxDB si hay WiFi
  if (WiFi.status() == WL_CONNECTED) {
    sendToInfluxDB(payload);
  } else {
    Serial.println("WiFi no conectado - Datos no enviados");
  }
}

void sendToInfluxDB(String payload) {
  HTTPClient http;
  http.begin(influxServer);
  http.addHeader("Authorization", "Token " + String(influxToken));
  http.addHeader("Content-Type", "text/plain");

  Serial.println("Enviando a InfluxDB: " + payload);
  int httpCode = http.POST(payload);
  
  if (httpCode == HTTP_CODE_OK) {
    Serial.println("Envío exitoso a InfluxDB");
  } else {
    Serial.print("Error HTTP: ");
    Serial.println(http.errorToString(httpCode));
  }
  
  http.end();
}

void processAndSendUltrasonico(String data) {
  // Ya no buscamos ninguna coma. El dato es solo la distancia en texto.
  String distancia = data;

  String payload = "sensores,ubicacion=habitacion,tipo=ULTRASONICO distancia=" + distancia;


  Serial.println("Enviando distancia ultrasónica a InfluxDB: " + distancia);

  if (WiFi.status() == WL_CONNECTED) {
    sendToInfluxDB(payload);
  } else {
    Serial.println("WiFi no conectado - Datos no enviados");
  }
}
