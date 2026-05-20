// ESP32 UART2
#define RXD2 16
#define TXD2 17

void setup() {
    Serial.begin(600);
    Serial2.begin(600, SERIAL_8N1, RXD2, TXD2);

    Serial.println("ESP32 listo");
}

void loop() {
    // Enviar datos
    Serial2.print("S");
    Serial.println("S");

    // Leer si hay datos
    /*if (Serial2.available()) {
        String data = Serial2.readStringUntil('\n');
        Serial.print("Recibido: ");
        Serial.println(data);
    }*/

    delay(100);
}
