/*****************************************************************************
 *  ESP32 #2 — FERMENTATION NODE — SHT21 Variant
 *  ===============================================
 *
 *  Role:
 *    - Simulates the fermentation area of a croissant bakery plant
 *    - Reads local sensors (SHT21 via I2C, MQ135 CO2 sensor)
 *    - Sends data periodically via CAN bus (ID 0x200)
 *    - No WiFi (CAN-only node)
 *
 *  CAN ID: 0x200
 *
 *  Hardware:
 *    - ESP32-S3
 *    - WCMCU-230 CAN transceiver (GPIO17 TX, GPIO18 RX)
 *    - SHT21  (I2C: SDA=GPIO8, SCL=GPIO9)
 *    - MQ135 on GPIO5 (analog input)
 *
 *  Required Libraries:
 *    - ESP32 Arduino Core (built-in TWAI driver, built-in Wire)
 *
 *  Installation:
 *    1. Install ESP32 board support in Arduino IDE
 *    2. Select "ESP32S3 Dev Module" as board
 *    3. Ensure 4.7kΩ pull-ups on SDA/SCL
 *    4. Upload to ESP32 #2
 *****************************************************************************/

#include <driver/twai.h>
#include <Wire.h>

/* ========================== CONFIGURATION ========================== */

// --- CAN Bus ---
const uint32_t CAN_TX_GPIO  = GPIO_NUM_17;
const uint32_t CAN_RX_GPIO  = GPIO_NUM_18;

// --- I2C pins for SHT21 ---
const uint8_t I2C_SDA = GPIO_NUM_8;
const uint8_t I2C_SCL = GPIO_NUM_9;

// --- Sensor Pins ---
const uint8_t PIN_MQ135  = GPIO_NUM_5;

// --- SHT21 I2C Address ---
const uint8_t SHT21_ADDR = 0x40;

// --- Node CAN ID ---
const uint32_t CAN_ID_FERMENTATION = 0x200;

// --- Timing Intervals (milliseconds) ---
const uint32_t INTERVAL_READ_SEND  = 30000;
const uint32_t INTERVAL_CAN_STATUS = 60000;

/* ======================== DATA STRUCTURES ========================== */

typedef struct {
  float    temperatura;
  float    humedad;
  bool     sensor_ok;
  uint16_t co2;
  uint32_t timestamp;
} fermentation_sensor_data_t;

fermentation_sensor_data_t sensorData = {0};

unsigned long lastSensorRead = 0;
unsigned long lastCanStatus  = 0;

/* ================================================================ */
/*                    TIMESTAMP HELPER                                */
/* ================================================================ */

void getTimestamp(char* buffer, size_t len) {
  unsigned long ms = millis();
  unsigned long sec  = ms / 1000;
  unsigned long min  = sec / 60;
  unsigned long hour = min / 60;
  snprintf(buffer, len, "%02lu:%02lu:%02lu",
           hour % 24, min % 60, sec % 60);
}

/* ================================================================ */
/*                    SHT21 DRIVER (I2C)                              */
/* ================================================================ */

bool readSHT21_Temperature(float* temp) {
  Wire.beginTransmission(SHT21_ADDR);
  Wire.write(0xF3);
  if (Wire.endTransmission() != 0) return false;

  delay(85);

  if (Wire.requestFrom(SHT21_ADDR, (uint8_t)3) != 3) return false;

  uint16_t raw = Wire.read() << 8;
  raw |= Wire.read();
  Wire.read();

  raw &= 0xFFFC;
  *temp = -46.85f + 175.72f * (raw / 65536.0f);
  return true;
}

bool readSHT21_Humidity(float* hum) {
  Wire.beginTransmission(SHT21_ADDR);
  Wire.write(0xF5);
  if (Wire.endTransmission() != 0) return false;

  delay(30);

  if (Wire.requestFrom(SHT21_ADDR, (uint8_t)3) != 3) return false;

  uint16_t raw = Wire.read() << 8;
  raw |= Wire.read();
  Wire.read();

  raw &= 0xFFFC;
  *hum = -6.0f + 125.0f * (raw / 65536.0f);
  return true;
}

bool readSHT21(float* temp, float* hum) {
  if (!readSHT21_Temperature(temp)) return false;
  if (!readSHT21_Humidity(hum))     return false;
  return true;
}

/* ================================================================ */
/*                    TWAI (CAN) SETUP                               */
/* ================================================================ */

void setupCAN() {
  twai_general_config_t gConfig = TWAI_GENERAL_CONFIG_DEFAULT(
    (gpio_num_t)CAN_TX_GPIO,
    (gpio_num_t)CAN_RX_GPIO,
    TWAI_MODE_NORMAL
  );
  twai_timing_config_t tConfig = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t fConfig = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  Serial.println("[CAN] Instalando driver TWAI...");
  esp_err_t err = twai_driver_install(&gConfig, &tConfig, &fConfig);
  if (err == ESP_OK) {
    Serial.println("[CAN] Driver instalado");
  } else {
    Serial.printf("[CAN] ERROR instalando driver: %d\n", err);
    return;
  }

  err = twai_start();
  if (err == ESP_OK) {
    Serial.println("[CAN] Controlador iniciado (500 kbps)");
  } else {
    Serial.printf("[CAN] ERROR iniciando: %d\n", err);
  }
}

/* ================================================================ */
/*                    SHT21 SENSOR READING                           */
/* ================================================================ */

void readSHT21_Sensor() {
  float temp, hum;
  sensorData.sensor_ok = readSHT21(&temp, &hum);

  if (sensorData.sensor_ok) {
    sensorData.temperatura = temp;
    sensorData.humedad     = hum;
    Serial.printf("[SHT21] Temp: %.1f C, Hum: %.1f %%\n", temp, hum);
  } else {
    sensorData.temperatura = 0.0f;
    sensorData.humedad     = 0.0f;
    Serial.println("[SHT21] ERROR: No se pudo leer el sensor");
    Serial.println("[SHT21] Verifica cableado I2C (SDA=GPIO8, SCL=GPIO9)");
  }
}

/* ================================================================ */
/*                    MQ135 CO2 SENSOR READING                       */
/* ================================================================ */

void readMQ135() {
  int raw = analogRead(PIN_MQ135);
  sensorData.co2 = (uint16_t)raw;
  Serial.printf("[MQ135] Valor ADC: %d\n", raw);
}

/* ================================================================ */
/*                    SENSOR READ ROUTINE                            */
/* ================================================================ */

void readAllSensors() {
  Serial.println("--- Leyendo sensores de fermentación ---");
  readSHT21_Sensor();
  readMQ135();
  sensorData.timestamp = millis();
  Serial.println("--- Lectura completada ---");
}

/* ================================================================ */
/*                    CAN TRANSMIT                                   */
/* ================================================================ */

void transmitSensorData() {
  twai_message_t frame;
  frame.identifier       = CAN_ID_FERMENTATION;
  frame.extd             = 0;
  frame.rtr              = 0;
  frame.data_length_code = 8;

  int16_t tempInt = (int16_t)(sensorData.temperatura * 10);
  frame.data[0] = (uint8_t)(tempInt >> 8);
  frame.data[1] = (uint8_t)(tempInt & 0xFF);

  uint16_t humInt = (uint16_t)(sensorData.humedad * 10);
  frame.data[2] = (uint8_t)(humInt >> 8);
  frame.data[3] = (uint8_t)(humInt & 0xFF);

  frame.data[4] = (uint8_t)(sensorData.co2 >> 8);
  frame.data[5] = (uint8_t)(sensorData.co2 & 0xFF);

  frame.data[6] = 0;
  if (!sensorData.sensor_ok) frame.data[6] |= 0x01;

  frame.data[7] = 0x00;

  esp_err_t err = twai_transmit(&frame, pdMS_TO_TICKS(100));
  if (err == ESP_OK) {
    Serial.printf("[CAN TX] ID=0x%03X Enviado (%d bytes)\n",
                  frame.identifier, frame.data_length_code);
  } else {
    Serial.printf("[CAN TX] ERROR: %d\n", err);
  }
}

/* ================================================================ */
/*                    CAN RECEIVE                                     */
/* ================================================================ */

void receiveCANMessages() {
  twai_message_t frame;
  esp_err_t err = twai_receive(&frame, pdMS_TO_TICKS(0));

  if (err == ESP_OK) {
    Serial.printf("[CAN RX] ID=0x%03X DLC=%d ",
                  frame.identifier, frame.data_length_code);
    for (int i = 0; i < frame.data_length_code; i++) {
      Serial.printf("%02X ", frame.data[i]);
    }
    Serial.println();
  }
}

/* ================================================================ */
/*                    CAN DIAGNOSTICS                                */
/* ================================================================ */

void printCANStatus() {
  twai_status_info_t status;
  esp_err_t err = twai_get_status_info(&status);
  if (err == ESP_OK) {
    Serial.println("--- Estado CAN ---");
    Serial.printf("Estado: %d\n", status.state);
    Serial.printf("TX Errores: %lu\n", status.tx_error_counter);
    Serial.printf("RX Errores: %lu\n", status.rx_error_counter);
    Serial.printf("TX Fracasos: %lu\n", status.tx_failed_count);
    Serial.printf("RX Perdidos: %lu\n", status.rx_missed_count);
    Serial.printf("Errores Bus: %d\n", status.bus_error_count);
  }
}

/* ================================================================ */
/*                    SERIAL DIAGNOSTICS                             */
/* ================================================================ */

void printSerialDiagnostics() {
  char timestamp[16];
  getTimestamp(timestamp, sizeof(timestamp));

  Serial.println("\n========== FERMENTATION NODE REPORT ==========");
  Serial.printf("Timestamp: %s\n", timestamp);

  Serial.println("{");
  Serial.println("  \"area\": \"fermentation\",");
  Serial.println("  \"sensor\": \"SHT21\",");
  Serial.printf("  \"temperatura\": %.1f,\n", sensorData.temperatura);
  Serial.printf("  \"humedad\": %.1f,\n", sensorData.humedad);
  Serial.printf("  \"co2\": %d,\n", sensorData.co2);
  Serial.printf("  \"estado\": \"%s\"\n",
                sensorData.sensor_ok ? "ok" : "error");
  Serial.println("}");
  Serial.println("==============================================\n");
}

/* ================================================================ */
/*                    SETUP                                          */
/* ================================================================ */

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n============================================");
  Serial.println("  ESP32 #2 — FERMENTATION NODE");
  Serial.println("  Sensor: SHT21 (I2C)");
  Serial.println("  CAN ID: 0x200");
  Serial.println("============================================");

  // Initialize I2C for SHT21
  Wire.begin(I2C_SDA, I2C_SCL);
  Serial.printf("[I2C] Inicializado: SDA=GPIO%d, SCL=GPIO%d\n",
                I2C_SDA, I2C_SCL);

  pinMode(PIN_MQ135, INPUT);
  Serial.println("[MQ135] Inicializado en GPIO5");

  setupCAN();

  Serial.println("=== SISTEMA LISTO ===");
}

/* ================================================================ */
/*                    MAIN LOOP                                      */
/* ================================================================ */

void loop() {
  unsigned long now = millis();

  if (now - lastSensorRead >= INTERVAL_READ_SEND) {
    lastSensorRead = now;
    readAllSensors();
    transmitSensorData();
    printSerialDiagnostics();
  }

  receiveCANMessages();

  if (now - lastCanStatus >= INTERVAL_CAN_STATUS) {
    lastCanStatus = now;
    printCANStatus();
  }
}
