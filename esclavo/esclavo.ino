// ESP32 Esclavo
#define RXD2 16
#define TXD2 17
#define LED 2


void setup() {
    Serial.begin(600);
    Serial2.begin(600, SERIAL_8N1, RXD2, TXD2);

    Serial.println("ESP32 listo");
}

void loop() {
    // Enviar datos
    //Serial2.print("A");
    //Serial.println("A");

    // Leer si hay datos
    if (Serial2.available()) {
        char data = Serial2.read();
        Serial.print("Recibido: ");
        Serial.println(data);
    }

    //delay(100);
}
