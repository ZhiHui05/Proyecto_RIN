// PRÁCTICA 1 - COMUNICACIÓN SERIE UART
// ESP32 recibe datos enviados desde el PC

#define RXD1 16
#define TXD1 15

void setup() {
  // Monitor serie (USB)
  Serial.begin(115200);
  while (!Serial);

  // UART1
  Serial1.begin(9600, SERIAL_8N1, RXD1, TXD1);

  Serial.println("ESP32 lista para recibir datos por UART");
}

void loop() {
  // Comprobar si hay datos disponibles
  if (Serial1.available()) {
    char dato = Serial1.read();   // Leer 1 byte
    Serial.print("Dato recibido: ");
    Serial.println(dato);
  }
}