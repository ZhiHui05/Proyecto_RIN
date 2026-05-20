#include <WiFi.h>
#include <DHT.h>
#include <WiFiClient.h>

#define DHTPIN 4
#define DHTTYPE DHT11
#define HW038_PIN 34

// Credenciales WiFi
const char* ssid = "MIWIFI_2G_nyEG";
const char* password = "jUbaZ9Vy";
const char* receiverIP = "192.168.1.132";  // <-- CAMBIAR IP DEL RECEPTOR
const uint16_t serverPort = 5000;

DHT dht(DHTPIN, DHTTYPE);
WiFiClient client;
bool connectedToReceiver = false;
bool ackRecibido = false;

// Estructura de datos (Debe ser idéntica en el receptor)
typedef struct struct_message {
  float temperatura;
  float humedad;
  bool estado_dht;
  int nivel_hw;
  int valor_analogico_hw;
} struct_message;

struct_message myData;

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n\n=== INICIANDO EMISOR (CLIENTE WiFi) ===");
  
  dht.begin();

  // Conectar a WiFi
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  Serial.print("Conectando a: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  Serial.println("");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("✓ WiFi conectado");
    Serial.print("IP del EMISOR: ");
    Serial.println(WiFi.localIP());
    Serial.print("Conectando a receptor: ");
    Serial.print(receiverIP);
    Serial.print(":");
    Serial.println(serverPort);
  } else {
    Serial.println("✗ Error: No se pudo conectar a WiFi");
  }
}

void loop() {
  // Intentar conectar al receptor si no estamos conectados
  if (!connectedToReceiver || !client.connected()) {
    if (client.connected()) client.stop();
    
    Serial.print("Conectando a receptor ");
    Serial.print(receiverIP);
    Serial.print(":");
    Serial.println(serverPort);
    
    if (client.connect(receiverIP, serverPort)) {
      connectedToReceiver = true;
      Serial.println("✓ Conectado al receptor");
    } else {
      Serial.println("✗ Fallo de conexión - reintentando en 5s");
      delay(5000);
      return;
    }
  }

  // 1. Leer sensor DHT11
  myData.temperatura = dht.readTemperature();
  myData.humedad = dht.readHumidity();
  
  if (isnan(myData.temperatura) || isnan(myData.humedad)) {
    myData.estado_dht = false;
    Serial.println("⚠ Falla al leer DHT11");
  } else {
    myData.estado_dht = true;
    Serial.print("📊 DHT11: ");
    Serial.print(myData.temperatura);
    Serial.print("°C, ");
    Serial.print(myData.humedad);
    Serial.println("%");
  }

  // 2. Leer sensor HW038
  myData.valor_analogico_hw = analogRead(HW038_PIN);
  myData.nivel_hw = map(myData.valor_analogico_hw, 0, 4095, 100, 0);

  Serial.print("💧 HW038: ");
  Serial.print(myData.nivel_hw);
  Serial.println("%");
  Serial.println("(Analogico: ");
  Serial.print(myData.valor_analogico_hw);
  Serial.println(")");

  // 3. Enviar datos por WiFi al receptor
  Serial.print("📤 Enviando paquete... ");
  ackRecibido = false;
  
  if (client.write((uint8_t *)&myData, sizeof(myData)) == sizeof(myData)) {
    Serial.println("enviado (esperando ACK)");
    
    // Esperar ACK del receptor
    int espera = 0;
    while (!ackRecibido && espera < 100 && client.available() == 0) {
      delay(10);
      espera++;
    }
    
    if (client.available()) {
      uint8_t ack = client.read();
      if (ack == 0xAA) {
        Serial.println("  ✓✓✓ CONFIRMACIÓN RECIBIDA");
        ackRecibido = true;
      }
    }
    
    if (!ackRecibido) {
      Serial.println("  ✗ TIMEOUT - Sin respuesta del receptor");
    }
  } else {
    Serial.println("ERROR en envío");
    connectedToReceiver = false;
  }

  Serial.println("---");
  delay(5000);
}
