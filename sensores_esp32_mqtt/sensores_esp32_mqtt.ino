#include <WiFi.h>
#include <PubSubClient.h>
#include <esp_now.h>
#include <WiFiServer.h>
#include <WiFiClient.h>

// Credenciales WiFi y MQTT
//const char* ssid = "iPhone de ZhiHui";
//const char* password = "ZhiHuiLi2005";
//const char* ssid = "MIWIFI_2G_nyEG";
//const char* password = "jUbaZ9Vy";

const char* ssid = "UPV-PSK";               // SSID de la red WiFi
const char* password = "C0n3ct4nd0s3_m0l4!";  // Contraseña de la red WiFi

const char* mqtt_server = "broker.mqtt-dashboard.com";

WiFiClient espClient;
PubSubClient client(espClient);
WiFiServer server(5000);  // Servidor TCP en puerto 5000
WiFiClient serverClient;

// Estructura de datos (Debe ser idéntica en el emisor)
typedef struct struct_message {
  float temperatura;
  float humedad;
  bool estado_dht;
  int nivel_hw;
  int valor_analogico_hw;
} struct_message;

struct_message myData;
bool newData = false; // Bandera para saber si hay nuevos datos

// Callback de ESP-NOW (NO USADO - comunicación por WiFi)
/*
void OnDataRecv(const esp_now_recv_info_t * info, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));
  newData = true;
  
  Serial.print("✓ Paquete recibido de: ");
  for(int i = 0; i < 6; i++) {
    if(info->src_addr[i] < 0x10) Serial.print("0");
    Serial.print(info->src_addr[i], HEX);
    if(i < 5) Serial.print(":");
  }
  Serial.println(" - Enviando ACK...");
  
  uint8_t ack = 0xAA;
  esp_now_send(info->src_addr, &ack, 1);
}
*/

void setup_wifi() {
  // MODO STA (Importante para que convivan WiFi y ESP-NOW)
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();  // Limpiar conexiones previas
  delay(100);
  
  Serial.print("\nIntentando conectar a: ");
  Serial.println(ssid);
  Serial.println("Asegúrate que la red está en 2.4GHz (no 5GHz)");
  
  WiFi.begin(ssid, password);

  int attempts = 0;
  int maxAttempts = 40;  // 20 segundos
  
  while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
    delay(500);
    Serial.print(".");
    attempts++;
    
    // Mostrar estado cada 10 intentos
    if (attempts % 10 == 0) {
      Serial.print(" (");
      Serial.print(attempts);
      Serial.print("/");
      Serial.print(maxAttempts);
      Serial.print(")");
    }
  }

  Serial.println("");
  Serial.print("Estado WiFi: ");
  Serial.println(WiFi.status());
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("✓ WiFi conectado");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("Señal (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    Serial.print("MAC Address de este receptor (COPIA EN EL EMISOR): ");
    Serial.println(WiFi.macAddress()); // ¡Copia ESTA MAC en el código del Emisor!
  } else {
    Serial.println("✗ Error: No se pudo conectar a WiFi");
    Serial.println("\nProbables causas:");
    Serial.println("1. La red está en 5GHz (cambia a 2.4GHz)");
    Serial.println("2. Contraseña incorrecta");
    Serial.println("3. Red no disponible");
    Serial.println("4. ESP32 muy lejos del router");
    Serial.println("\nContinuando sin WiFi (ESP-NOW funcionará)");
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("Conectando MQTT...");
    if (client.connect("ESP32_RECEPTOR_NOW")) {
      Serial.println("MQTT conectado");
    } else {
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n\n=== INICIANDO RECEPTOR (SERVIDOR WiFi) ===");
  
  setup_wifi();
  
  client.setServer(mqtt_server, 1883);

  // Iniciar servidor TCP
  server.begin(5000);
  Serial.println("✓ Servidor TCP escuchando en puerto 5000");
  Serial.print("Conectar cliente a: ");
  Serial.print(WiFi.localIP());
  Serial.println(":5000");
  
  Serial.println("Sistema listo para recibir datos por WiFi");
}

void loop() {
  // Verificar conexión WiFi
  if (WiFi.status() != WL_CONNECTED) {
    static unsigned long lastWiFiCheck = 0;
    if (millis() - lastWiFiCheck > 30000) {
      lastWiFiCheck = millis();
      Serial.println("Intentando reconectar a WiFi...");
      WiFi.reconnect();
    }
  }

  // Aceptar conexiones de clientes
  if (server.hasClient()) {
    if (!serverClient || !serverClient.connected()) {
      if (serverClient) serverClient.stop();
      serverClient = server.available();
      Serial.print("✓ Cliente conectado desde: ");
      Serial.println(serverClient.remoteIP());
    } else {
      WiFiClient otherClient = server.available();
      otherClient.stop();
    }
  }

  // Recibir datos del cliente
  if (serverClient && serverClient.connected()) {
    if (serverClient.available() >= sizeof(myData)) {
      serverClient.readBytes((uint8_t *)&myData, sizeof(myData));
      newData = true;
      
      // Enviar confirmación al cliente
      uint8_t ack = 0xAA;
      serverClient.write(&ack, 1);
      Serial.println("✓ Datos recibidos por WiFi - ACK enviado");
    }
  }

  // Si recibimos datos, publicamos en MQTT
  if (newData) {
    myData.estado_dht = true;
    Serial.print("📊 DHT11 - Temperatura: ");
    Serial.print(myData.temperatura);
    Serial.print(" C, Humedad: ");
    Serial.print(myData.humedad);
    Serial.println(" %");

    // ---------------------------
    // DHT11
    // ---------------------------
    String jsonDHT = "{";
    jsonDHT += "\"temperatura\":" + String(myData.temperatura) + ",";
    jsonDHT += "\"humedad\":" + String(myData.humedad) + ",";
    jsonDHT += "\"estado\":" + String(myData.estado_dht ? "true" : "false");
    jsonDHT += "}";

    if (WiFi.status() == WL_CONNECTED && client.connected()) {
      client.publish("sensores/dht11", jsonDHT.c_str());
      Serial.println("  Publicado DHT11: " + jsonDHT);
    } else {
      Serial.println("  MQTT no disponible - solo recibido: " + jsonDHT);
    }

    // ---------------------------
    // HW038
    // ---------------------------
    String jsonHW = "{";
    jsonHW += "\"nivel\":" + String(myData.nivel_hw) + ",";
    jsonHW += "\"valor_analogico\":" + String(myData.valor_analogico_hw);
    jsonHW += "}";

    if (WiFi.status() == WL_CONNECTED && client.connected()) {
      client.publish("sensores/hw038", jsonHW.c_str());
      Serial.println("  Publicado HW038: " + jsonHW);
    } else {
      Serial.println("  Recibido HW038: " + jsonHW);
    }

    newData = false;
  }
  
  // Verificar conexión MQTT
  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      static unsigned long lastMqttCheck = 0;
      if (millis() - lastMqttCheck > 30000) {
        lastMqttCheck = millis();
        reconnect();
      }
    }
    client.loop();
  }
}