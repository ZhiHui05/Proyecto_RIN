#include <WiFi.h>
#include <PubSubClient.h>   // Librería para MQTT
#include <Wire.h>
#include <Adafruit_HTU21DF.h>
#include <ArduinoJson.h>

const char* ssid = "UPV-PSK";               // SSID de la red WiFi
const char* password = "C0n3ct4nd0s3_m0l4!";  // Contraseña de la red WiFi
const char* mqtt_server = "broker.mqtt-dashboard.com";  // Dirección del Broker MQTT (puedes usar el broker que prefieras)
const int mqtt_port = 1883;  // Puerto de conexión MQTT

WiFiClient espClient;
PubSubClient client(espClient);

Adafruit_HTU21DF htu = Adafruit_HTU21DF();

void setup() {
  Serial.begin(115200);  // Inicia la comunicación serial
  delay(10);

  if (!htu.begin()) {
    Serial.println("Error con HTU21D");
    while (1);
  }

  // Conectar a la red WiFi
  WiFi.begin(ssid, password);
  
  Serial.print("Conectando a WiFi");
  
  // Esperar hasta que esté conectado
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  // Mostrar la IP obtenida
  Serial.println("");
  Serial.println("Conectado a WiFi");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  
  // Conectar al broker MQTT
  client.setServer(mqtt_server, mqtt_port);
  reconnectMQTT();
}

void reconnectMQTT() {
  // Intentar conectar al broker MQTT
  while (!client.connected()) {
    Serial.print("Conectando al broker MQTT...");
    
    if (client.connect("ESP32Client")) {  // Nombre del cliente MQTT
      Serial.println("Conectado!");
    } else {
      Serial.print("falló, rc=");
      Serial.print(client.state());
      Serial.println(" intentando en 5 segundos...");
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
  
  // Obtener la temperatura
  float temperature = htu.readTemperature();
  
   // Crear un objeto JSON para enviar
  StaticJsonDocument<200> doc;  // Documento JSON de tamaño suficiente
  doc["sensor"] = "HTU21D";     // Nombre del sensor
  doc["temperatura"] = temperature;  // Valor de la temperatura
  
  // Convertir el objeto JSON a un string
  char jsonBuffer[512];  // Buffer para el JSON en formato texto
  serializeJson(doc, jsonBuffer);
  
  // Publicar el JSON en el topic "sensor/datos"
  client.publish("sensor/datos", jsonBuffer);  // Publica el objeto JSON como string

  // Esperar 10 segundos antes de la siguiente medición
  delay(1000);
}