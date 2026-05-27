/*****************************************************************************
 *  ESP32 #1 — MASTER NODE (Raw Materials Area) — SHT21 Variant
 *  ==============================================================
 *
 *  Role:
 *    - Reads local sensors from the raw materials area
 *    - Receives CAN messages from other ESP32 nodes
 *    - Connects to WiFi
 *    - Publishes MQTT messages with timestamped JSON
 *    - Acts as supervisory node
 *
 *  CAN ID: 0x100
 *
 *  Hardware:
 *    - ESP32-S3
 *    - WCMCU-230 CAN transceiver (GPIO17 TX, GPIO18 RX)
 *    - SHT21  (I2C: SDA=GPIO8, SCL=GPIO9)
 *    - LDR on GPIO5
 *    - Water level sensor on GPIO6
 *
 *  Required Libraries:
 *    - PubSubClient by Nick O'Leary (v2.8+)
 *    - ESP32 Arduino Core (built-in TWAI driver, built-in Wire)
 *
 *  Installation:
 *    1. Install ESP32 board support in Arduino IDE
 *    2. Install PubSubClient via Library Manager
 *    3. Make sure pull-up resistors (4.7kΩ) are present on SDA/SCL
 *    4. Select "ESP32S3 Dev Module" as board
 *    5. Upload to ESP32 #1
 *****************************************************************************/

#include <driver/twai.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>

/* ========================== CONFIGURATION ========================== */

// --- WiFi Credentials ---
const char* WIFI_SSID     = "ESP32S3";
const char* WIFI_PASSWORD = "lambardooo";

// --- MQTT Broker ---
const char* MQTT_SERVER   = "broker.mqtt-dashboard.com";
const int   MQTT_PORT     = 1883;
const char* MQTT_CLIENT_ID = "ESP32_MASTER_RAW_MATERIALS";

// --- CAN Bus ---
const uint32_t CAN_TX_GPIO  = GPIO_NUM_17;
const uint32_t CAN_RX_GPIO  = GPIO_NUM_18;

// --- I2C pins for SHT21 ---
const uint8_t I2C_SDA = GPIO_NUM_8;
const uint8_t I2C_SCL = GPIO_NUM_9;

// --- Sensor Pins ---
const uint8_t PIN_LDR         = GPIO_NUM_5;
const uint8_t PIN_WATER_LEVEL = GPIO_NUM_6;

// --- SHT21 I2C Address ---
const uint8_t SHT21_ADDR = 0x40;

// --- Timing Intervals (milliseconds) ---
const uint32_t INTERVAL_READ_SENSORS  = 30000;
const uint32_t INTERVAL_WIFI_CHECK    = 30000;
const uint32_t INTERVAL_MQTT_CHECK    = 30000;
const uint32_t INTERVAL_CAN_STATUS    = 60000;

// --- CAN Identifiers ---
const uint32_t CAN_ID_RAW_MATERIALS   = 0x100;
const uint32_t CAN_ID_FERMENTATION    = 0x200;
const uint32_t CAN_ID_BAKING          = 0x300;

/* ======================== GLOBAL OBJECTS =========================== */

WiFiClient   wifiClient;
PubSubClient mqttClient(wifiClient);

/* ======================== DATA STRUCTURES ========================== */

typedef struct {
  float   temperatura;
  float   humedad;
  bool    sensor_ok;
  uint8_t ldr_valor;
  uint8_t nivel_agua;
  uint32_t timestamp;
} raw_materials_data_t;

raw_materials_data_t localData = {0};

typedef struct {
  float temperatura;
  float humedad;
  uint16_t co2;
  bool     data_valida;
  uint32_t timestamp;
} fermentation_data_t;

typedef struct {
  float   temperatura;
  uint16_t flujo_aire;
  bool    data_valida;
  uint32_t timestamp;
} baking_data_t;

fermentation_data_t fermentData = {0};
baking_data_t       bakingData  = {0};

bool newCanMessage   = false;
bool sensorsRead     = false;

unsigned long lastSensorRead  = 0;
unsigned long lastWifiCheck   = 0;
unsigned long lastMqttCheck   = 0;
unsigned long lastCanStatus   = 0;

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

/**
 * SHT21 is a digital I2C temperature and humidity sensor.
 * Address: 0x40
 *
 * Commands:
 *   0xF3 — Measure Temperature (No Hold Master)
 *   0xF5 — Measure Humidity   (No Hold Master)
 *
 * Measure sequence:
 *   1. Send command byte
 *   2. Wait for conversion (85ms temp, 30ms hum)
 *   3. Read 3 bytes: MSB, LSB, CRC (CRC ignored here)
 *
 * Conversion formulas:
 *   T = -46.85 + 175.72 * (raw / 65536.0)
 *   RH = -6 + 125 * (raw / 65536.0)
 */

bool readSHT21_Temperature(float* temp) {
  Wire.beginTransmission(SHT21_ADDR);
  Wire.write(0xF3);  // Measure temperature (no hold)
  uint8_t err = Wire.endTransmission();
  if (err != 0) return false;

  delay(85);  // Max conversion time for 14-bit

  if (Wire.requestFrom(SHT21_ADDR, (uint8_t)3) != 3) return false;

  uint16_t raw = Wire.read() << 8;
  raw |= Wire.read();
  Wire.read();  // Discard CRC

  raw &= 0xFFFC;  // Clear status bits
  *temp = -46.85f + 175.72f * (raw / 65536.0f);
  return true;
}

bool readSHT21_Humidity(float* hum) {
  Wire.beginTransmission(SHT21_ADDR);
  Wire.write(0xF5);  // Measure humidity (no hold)
  uint8_t err = Wire.endTransmission();
  if (err != 0) return false;

  delay(30);  // Max conversion time for 12-bit

  if (Wire.requestFrom(SHT21_ADDR, (uint8_t)3) != 3) return false;

  uint16_t raw = Wire.read() << 8;
  raw |= Wire.read();
  Wire.read();  // Discard CRC

  raw &= 0xFFFC;  // Clear status bits
  *hum = -6.0f + 125.0f * (raw / 65536.0f);
  return true;
}

bool readSHT21(float* temp, float* hum) {
  if (!readSHT21_Temperature(temp)) return false;
  if (!readSHT21_Humidity(hum))     return false;
  return true;
}

/* ================================================================ */
/*                    WiFi SETUP & MANAGEMENT                        */
/* ================================================================ */

void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.print("\n[WiFi] Conectando a: ");
  Serial.println(WIFI_SSID);
  Serial.println("[WiFi] Asegúrate que la red está en 2.4 GHz");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  const int MAX_ATTEMPTS = 40;

  while (WiFi.status() != WL_CONNECTED && attempts < MAX_ATTEMPTS) {
    delay(500);
    Serial.print(".");
    attempts++;
    if (attempts % 10 == 0) {
      Serial.printf(" (%d/%d)", attempts, MAX_ATTEMPTS);
    }
  }

  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WiFi] Conectado");
    Serial.printf("[WiFi] IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("[WiFi] RSSI: %d dBm\n", WiFi.RSSI());
    Serial.printf("[WiFi] MAC: %s\n", WiFi.macAddress().c_str());
  } else {
    Serial.println("[WiFi] ERROR: No se pudo conectar");
    Serial.println("[WiFi] Continuando sin WiFi...");
  }
}

void checkWiFiConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastWifiCheck > INTERVAL_WIFI_CHECK) {
      lastWifiCheck = millis();
      Serial.println("[WiFi] Intentando reconexión...");
      WiFi.reconnect();
    }
  }
}

/* ================================================================ */
/*                    MQTT SETUP & MANAGEMENT                        */
/* ================================================================ */

void setupMQTT() {
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
}

void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.println("[MQTT] Intentando conectar...");
    if (mqttClient.connect(MQTT_CLIENT_ID)) {
      Serial.println("[MQTT] Conectado");
    } else {
      Serial.printf("[MQTT] Falló, rc=%d, reintentando en 5s\n",
                    mqttClient.state());
      delay(5000);
    }
  }
}

void checkMQTTConnection() {
  if (!mqttClient.connected()) {
    if (millis() - lastMqttCheck > INTERVAL_MQTT_CHECK) {
      lastMqttCheck = millis();
      reconnectMQTT();
    }
  }
  mqttClient.loop();
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
  twai_timing_config_t   tConfig = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t   fConfig = TWAI_FILTER_CONFIG_ACCEPT_ALL();

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
    Serial.printf("[CAN] ERROR iniciando controlador: %d\n", err);
  }
}

/* ================================================================ */
/*                    SHT21 SENSOR READING                           */
/* ================================================================ */

void readSHT21_Sensor() {
  float temp, hum;
  localData.sensor_ok = readSHT21(&temp, &hum);

  if (localData.sensor_ok) {
    localData.temperatura = temp;
    localData.humedad     = hum;
    Serial.printf("[SHT21] Temp: %.1f C, Hum: %.1f %%\n", temp, hum);
  } else {
    localData.temperatura = 0.0f;
    localData.humedad     = 0.0f;
    Serial.println("[SHT21] ERROR: No se pudo leer el sensor");
    Serial.println("[SHT21] Verifica cableado I2C (SDA=GPIO8, SCL=GPIO9)");
    Serial.println("[SHT21] y resistencias pull-up de 4.7kΩ");
  }
}

/* ================================================================ */
/*                    ANALOG SENSORS READING                         */
/* ================================================================ */

uint8_t readLDR() {
  int raw = analogRead(PIN_LDR);
  uint8_t value = map(raw, 0, 4095, 0, 255);
  Serial.printf("[LDR] Raw: %d, Mapeado: %d\n", raw, value);
  return value;
}

uint8_t readWaterLevel() {
  int raw = analogRead(PIN_WATER_LEVEL);
  uint8_t value = map(raw, 0, 4095, 0, 255);
  Serial.printf("[Agua] Raw: %d, Nivel: %d\n", raw, value);
  return value;
}

/* ================================================================ */
/*                    SENSOR READ ROUTINE                            */
/* ================================================================ */

void readAllSensors() {
  Serial.println("--- Leyendo sensores locales ---");
  readSHT21_Sensor();
  localData.ldr_valor    = readLDR();
  localData.nivel_agua   = readWaterLevel();
  localData.timestamp    = millis();
  sensorsRead = true;
  Serial.println("--- Lectura completada ---");
}

/* ================================================================ */
/*                    CAN TRANSMIT                                   */
/* ================================================================ */

void transmitLocalData() {
  twai_message_t frame;
  frame.identifier = CAN_ID_RAW_MATERIALS;
  frame.extd       = 0;
  frame.rtr        = 0;
  frame.data_length_code = 8;

  int16_t tempInt = (int16_t)(localData.temperatura * 10);
  frame.data[0] = (uint8_t)(tempInt >> 8);
  frame.data[1] = (uint8_t)(tempInt & 0xFF);

  uint16_t humInt = (uint16_t)(localData.humedad * 10);
  frame.data[2] = (uint8_t)(humInt >> 8);
  frame.data[3] = (uint8_t)(humInt & 0xFF);

  frame.data[4] = localData.ldr_valor;
  frame.data[5] = localData.nivel_agua;

  frame.data[6] = 0;
  if (!localData.sensor_ok) frame.data[6] |= 0x01;

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
/*                    CAN RECEIVE                                    */
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

    if (frame.identifier == CAN_ID_FERMENTATION && frame.data_length_code >= 7) {
      int16_t  tempRaw = (int16_t)((frame.data[0] << 8) | frame.data[1]);
      uint16_t humRaw  = (uint16_t)((frame.data[2] << 8) | frame.data[3]);
      uint16_t co2Raw  = (uint16_t)((frame.data[4] << 8) | frame.data[5]);

      fermentData.temperatura  = tempRaw / 10.0f;
      fermentData.humedad      = humRaw / 10.0f;
      fermentData.co2          = co2Raw;
      fermentData.data_valida  = !(frame.data[6] & 0x01);
      fermentData.timestamp    = millis();

      Serial.printf("[FERMENT] Temp: %.1f C, Hum: %.1f %%, CO2: %d ppm\n",
                    fermentData.temperatura, fermentData.humedad, fermentData.co2);
      newCanMessage = true;
    }

    else if (frame.identifier == CAN_ID_BAKING && frame.data_length_code >= 5) {
      int16_t  tempRaw = (int16_t)((frame.data[0] << 8) | frame.data[1]);
      uint16_t airflowRaw = (uint16_t)((frame.data[2] << 8) | frame.data[3]);

      bakingData.temperatura  = tempRaw / 10.0f;
      bakingData.flujo_aire   = airflowRaw;
      bakingData.data_valida  = !(frame.data[4] & 0x01);
      bakingData.timestamp    = millis();

      Serial.printf("[BAKING] Temp: %.1f C, Flujo: %d\n",
                    bakingData.temperatura, bakingData.flujo_aire);
      newCanMessage = true;
    }

    else {
      Serial.println("[CAN RX] ID desconocido o DLC insuficiente");
    }
  }
}

/* ================================================================ */
/*                    MQTT PUBLISH                                   */
/* ================================================================ */

void publishMQTTData() {
  if (WiFi.status() != WL_CONNECTED || !mqttClient.connected()) {
    Serial.println("[MQTT] No disponible, saltando publicación");
    return;
  }

  char timestamp[16];
  getTimestamp(timestamp, sizeof(timestamp));

  String jsonRaw = "{";
  jsonRaw += "\"area\":\"raw_materials\",";
  jsonRaw += "\"sensor\":\"SHT21\",";
  jsonRaw += "\"timestamp\":\"" + String(timestamp) + "\",";
  jsonRaw += "\"temperatura\":" + String(localData.temperatura) + ",";
  jsonRaw += "\"humedad\":" + String(localData.humedad) + ",";
  jsonRaw += "\"ldr\":" + String(localData.ldr_valor) + ",";
  jsonRaw += "\"nivel_agua\":" + String(localData.nivel_agua) + ",";
  jsonRaw += "\"estado\":\"" + String(localData.sensor_ok ? "ok" : "error") + "\"";
  jsonRaw += "}";

  if (mqttClient.publish("sensores/raw_materials", jsonRaw.c_str())) {
    Serial.println("[MQTT] Publicado raw_materials: " + jsonRaw);
  } else {
    Serial.println("[MQTT] ERROR publicando raw_materials");
  }

  if (fermentData.data_valida) {
    String jsonFerment = "{";
    jsonFerment += "\"area\":\"fermentation\",";
    jsonFerment += "\"timestamp\":\"" + String(timestamp) + "\",";
    jsonFerment += "\"temperatura\":" + String(fermentData.temperatura) + ",";
    jsonFerment += "\"humedad\":" + String(fermentData.humedad) + ",";
    jsonFerment += "\"co2\":" + String(fermentData.co2);
    jsonFerment += "}";

    if (mqttClient.publish("sensores/fermentation", jsonFerment.c_str())) {
      Serial.println("[MQTT] Publicado fermentation: " + jsonFerment);
    }
  }

  if (bakingData.data_valida) {
    String jsonBaking = "{";
    jsonBaking += "\"area\":\"baking\",";
    jsonBaking += "\"timestamp\":\"" + String(timestamp) + "\",";
    jsonBaking += "\"temperatura\":" + String(bakingData.temperatura) + ",";
    jsonBaking += "\"flujo_aire\":" + String(bakingData.flujo_aire);
    jsonBaking += "}";

    if (mqttClient.publish("sensores/baking", jsonBaking.c_str())) {
      Serial.println("[MQTT] Publicado baking: " + jsonBaking);
    }
  }
}

/* ================================================================ */
/*                    SERIAL DIAGNOSTICS                             */
/* ================================================================ */

void printSerialJSON() {
  char timestamp[16];
  getTimestamp(timestamp, sizeof(timestamp));

  Serial.println("\n========== REPORTE COMPLETO ==========");
  Serial.printf("Timestamp: %s\n", timestamp);

  Serial.println("{");
  Serial.println("  \"area\": \"raw_materials\",");
  Serial.println("  \"sensor\": \"SHT21\",");
  Serial.printf("  \"temperatura\": %.1f,\n", localData.temperatura);
  Serial.printf("  \"humedad\": %.1f,\n", localData.humedad);
  Serial.printf("  \"ldr\": %d,\n", localData.ldr_valor);
  Serial.printf("  \"nivel_agua\": %d,\n", localData.nivel_agua);
  Serial.printf("  \"estado\": \"%s\"\n", localData.sensor_ok ? "ok" : "error");
  Serial.println("}");

  if (fermentData.data_valida) {
    Serial.println("{");
    Serial.println("  \"area\": \"fermentation\",");
    Serial.printf("  \"temperatura\": %.1f,\n", fermentData.temperatura);
    Serial.printf("  \"humedad\": %.1f,\n", fermentData.humedad);
    Serial.printf("  \"co2\": %d\n", fermentData.co2);
    Serial.println("}");
  }

  if (bakingData.data_valida) {
    Serial.println("{");
    Serial.println("  \"area\": \"baking\",");
    Serial.printf("  \"temperatura\": %.1f,\n", bakingData.temperatura);
    Serial.printf("  \"flujo_aire\": %d\n", bakingData.flujo_aire);
    Serial.println("}");
  }

  Serial.println("=======================================\n");
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
/*                    SETUP                                          */
/* ================================================================ */

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n============================================");
  Serial.println("  ESP32 #1 — MASTER NODE (Raw Materials)");
  Serial.println("  Sensor: SHT21 (I2C)");
  Serial.println("============================================");

  // Initialize I2C for SHT21
  Wire.begin(I2C_SDA, I2C_SCL);
  Serial.printf("[I2C] Inicializado: SDA=GPIO%d, SCL=GPIO%d\n",
                I2C_SDA, I2C_SCL);

  // Configure analog pins
  pinMode(PIN_LDR, INPUT);
  pinMode(PIN_WATER_LEVEL, INPUT);
  Serial.println("[SENSORES] Pines analógicos configurados");

  // Initialize CAN
  setupCAN();

  // Initialize WiFi
  setupWiFi();

  // Initialize MQTT
  setupMQTT();

  Serial.println("=== SISTEMA LISTO ===");
}

/* ================================================================ */
/*                    MAIN LOOP                                      */
/* ================================================================ */

void loop() {
  unsigned long now = millis();

  if (now - lastSensorRead >= INTERVAL_READ_SENSORS) {
    lastSensorRead = now;
    readAllSensors();
    transmitLocalData();
  }

  receiveCANMessages();

  if (newCanMessage) {
    printSerialJSON();
    publishMQTTData();
    newCanMessage = false;
  }

  if (sensorsRead) {
    printSerialJSON();
    publishMQTTData();
    sensorsRead = false;
  }

  checkWiFiConnection();

  if (WiFi.status() == WL_CONNECTED) {
    checkMQTTConnection();
  }

  if (now - lastCanStatus >= INTERVAL_CAN_STATUS) {
    lastCanStatus = now;
    printCANStatus();
  }
}
