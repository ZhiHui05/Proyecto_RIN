#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_HTU21DF.h>

const char* ssid = "UPV-PSK";               // SSID de la red WiFi
const char* password = "C0n3ct4nd0s3_m0l4!";  // Contraseña de la red WiFi
WebServer server(80);
Adafruit_HTU21DF htu = Adafruit_HTU21DF();

void handleTemp() {
  float t = htu.readTemperature();
  server.send(200, "text/plain", String(t));
}

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


  server.on("/temp", handleTemp);
  server.begin();
}

void loop() {
  server.handleClient();
}