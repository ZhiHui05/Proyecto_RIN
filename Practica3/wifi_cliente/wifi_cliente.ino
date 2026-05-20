#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "UPV-PSK";               // SSID de la red WiFi
const char* password = "C0n3ct4nd0s3_m0l4!";  // Contraseña de la red WiFi

String serverIP = "172.18.191.161";   // <-- CAMBIAR

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.print("Conectando");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConectado!");
}

void loop() {

  HTTPClient http;
  String url = "http://" + serverIP + "/temp";

  http.begin(url);
  int httpCode = http.GET();

  if (httpCode > 0) {
    String payload = http.getString();
    Serial.print("Temperatura recibida: ");
    Serial.println(payload);
  } else {
    Serial.println("Error en petición");
  }

  http.end();
  delay(5000);
}