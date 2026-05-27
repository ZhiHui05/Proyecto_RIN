/*****************************************************************************
 *  ESP32 #3 — BAKING NODE
 *  =========================
 *
 *  Role:
 *    - Simulates the baking area of a croissant bakery plant
 *    - Reads local sensors (thermocouple, airflow potentiometer)
 *    - Sends data periodically via CAN bus (ID 0x300)
 *    - No WiFi (CAN-only node)
 *
 *  CAN ID: 0x300
 *
 *  Hardware:
 *    - ESP32-S3
 *    - WCMCU-230 CAN transceiver (GPIO17 TX, GPIO18 RX)
 *    - Thermocouple (MAX6675) on GPIO4 (or simulated analog input)
 *    - Airflow potentiometer on GPIO5 (analog input)
 *
 *  Required Libraries:
 *    - ESP32 Arduino Core (built-in TWAI driver)
 *
 *  Note on Thermocouple:
 *    If using a MAX6675 module, install the "Adafruit MAX6675" library
 *    and connect: SO->GPIO19, CS->GPIO23, SCK->GPIO18 (shared with CAN?)
 *    Since GPIO18 is used by CAN, this academic version reads the
 *    thermocouple from an analog input (GPIO4) as a simulation.
 *    For real MAX6675, use SPI on different pins or a separate ESP32.
 *
 *  Installation:
 *    1. Install ESP32 board support in Arduino IDE
 *    2. Select "ESP32S3 Dev Module" as board
 *    3. Upload to ESP32 #3
 *****************************************************************************/

#include <driver/twai.h>

/* ========================== CONFIGURATION ========================== */

// --- CAN Bus Timing ---
const uint32_t CAN_TX_GPIO  = GPIO_NUM_17;
const uint32_t CAN_RX_GPIO  = GPIO_NUM_18;

// --- Sensor Pins ---
const uint8_t PIN_THERMOCOUPLE = GPIO_NUM_4;    // Analog input (simulated)
const uint8_t PIN_AIRFLOW      = GPIO_NUM_5;    // Potentiometer analog input

// --- Node CAN ID ---
const uint32_t CAN_ID_BAKING = 0x300;

// --- Timing Intervals (milliseconds) ---
const uint32_t INTERVAL_READ_SEND  = 30000;  // 30 seconds
const uint32_t INTERVAL_CAN_STATUS = 60000;  // 60 seconds

/* ======================== DATA STRUCTURES ========================== */

typedef struct {
  float    temperatura;       // Thermocouple temperature in °C
  uint16_t flujo_aire;        // Airflow potentiometer reading (0-4095)
  bool     thermo_ok;         // Thermocouple status flag
  uint32_t timestamp;         // millis() when read
} baking_sensor_data_t;

baking_sensor_data_t sensorData = {0};

/* ====================== NON-BLOCKING TIMERS ======================= */

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
/*                    TWAI (CAN) SETUP                               */
/* ================================================================ */

/**
 * Configures and installs the ESP32 TWAI driver for CAN communication.
 *
 * Wiring:
 *   ESP32 GPIO17 -> WCMCU-230 TXD
 *   ESP32 GPIO18 -> WCMCU-230 RXD
 *
 * Bit rate: 500 kbps
 *
 * This node operates in NORMAL mode (transmit and receive).
 */
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
/*                    THERMOCOUPLE SENSOR READING                    */
/* ================================================================ */

/**
 * Reads the thermocouple temperature sensor.
 *
 * In this academic implementation, the thermocouple is simulated
 * using an analog input. The ADC value (0-4095) is mapped to a
 * realistic baking temperature range (25°C - 250°C).
 *
 * For a real MAX6675 thermocouple module, replace this function
 * with SPI communication using the Adafruit MAX6675 library.
 */
void readThermocouple() {
  int raw = analogRead(PIN_THERMOCOUPLE);    // 0-4095

  // Map ADC reading to baking temperature range
  sensorData.temperatura = map(raw, 0, 4095, 250, 2500) / 10.0f;

  // Validate reading
  if (raw < 10) {
    sensorData.thermo_ok = false;
    Serial.println("[THERMO] ERROR: Sensor desconectado o cortocircuito");
  } else {
    sensorData.thermo_ok = true;
    Serial.printf("[THERMO] Temp: %.1f C (ADC: %d)\n",
                  sensorData.temperatura, raw);
  }
}

/* ================================================================ */
/*                    AIRFLOW SENSOR READING                         */
/* ================================================================ */

/**
 * Reads the airflow potentiometer.
 * The potentiometer simulates an airflow sensor in the baking oven.
 * Raw ADC value (0-4095) is transmitted directly.
 */
void readAirflow() {
  int raw = analogRead(PIN_AIRFLOW);         // 0-4095
  sensorData.flujo_aire = (uint16_t)raw;
  Serial.printf("[AIRFLOW] Potenciómetro: %d\n", raw);
}

/* ================================================================ */
/*                    SENSOR READ ROUTINE                            */
/* ================================================================ */

/**
 * Reads all baking sensors at once and updates the data structure.
 * Called every INTERVAL_READ_SEND milliseconds.
 */
void readAllSensors() {
  Serial.println("--- Leyendo sensores de horneado ---");
  readThermocouple();
  readAirflow();
  sensorData.timestamp = millis();
  Serial.println("--- Lectura completada ---");
}

/* ================================================================ */
/*                    CAN TRANSMIT                                   */
/* ================================================================ */

/**
 * Encodes baking sensor data into a CAN frame and transmits it.
 *
 * CAN Frame Structure (ID = 0x300, 8 data bytes):
 *   Byte 0-1: Temperatura como int16 (valor * 10)
 *   Byte 2-3: Flujo de aire como uint16 (0-4095)
 *   Byte 4:   Status byte (bit0: thermocouple error)
 *   Byte 5-7: Reserved
 */
void transmitSensorData() {
  twai_message_t frame;
  frame.identifier       = CAN_ID_BAKING;
  frame.extd             = 0;            // Standard 11-bit ID
  frame.rtr              = 0;            // Data frame
  frame.data_length_code = 8;            // 8 bytes

  // Encode temperature as int16, scaled by 10
  int16_t tempInt = (int16_t)(sensorData.temperatura * 10);
  frame.data[0] = (uint8_t)(tempInt >> 8);
  frame.data[1] = (uint8_t)(tempInt & 0xFF);

  // Airflow raw value
  frame.data[2] = (uint8_t)(sensorData.flujo_aire >> 8);
  frame.data[3] = (uint8_t)(sensorData.flujo_aire & 0xFF);

  // Status byte
  frame.data[4] = 0;
  if (!sensorData.thermo_ok) frame.data[4] |= 0x01;

  frame.data[5] = 0x00;  // Reserved
  frame.data[6] = 0x00;  // Reserved
  frame.data[7] = 0x00;  // Reserved

  esp_err_t err = twai_transmit(&frame, pdMS_TO_TICKS(100));
  if (err == ESP_OK) {
    Serial.printf("[CAN TX] ID=0x%03X Enviado (%d bytes)\n",
                  frame.identifier, frame.data_length_code);
  } else {
    Serial.printf("[CAN TX] ERROR: %d\n", err);
  }
}

/* ================================================================ */
/*                    CAN RECEIVE (optional)                         */
/* ================================================================ */

/**
 * Checks for incoming CAN messages (non-blocking).
 * This node primarily transmits, but may receive broadcast commands.
 */
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

  Serial.println("\n========== BAKING NODE REPORT ==========");
  Serial.printf("Timestamp: %s\n", timestamp);

  Serial.println("{");
  Serial.println("  \"area\": \"baking\",");
  Serial.printf("  \"temperatura\": %.1f,\n", sensorData.temperatura);
  Serial.printf("  \"flujo_aire\": %d,\n", sensorData.flujo_aire);
  Serial.printf("  \"estado_thermo\": \"%s\"\n",
                sensorData.thermo_ok ? "ok" : "error");
  Serial.println("}");
  Serial.println("========================================\n");
}

/* ================================================================ */
/*                    SETUP                                          */
/* ================================================================ */

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n============================================");
  Serial.println("  ESP32 #3 — BAKING NODE");
  Serial.println("  CAN ID: 0x300");
  Serial.println("============================================");

  // Configure sensor pins
  pinMode(PIN_THERMOCOUPLE, INPUT);
  pinMode(PIN_AIRFLOW, INPUT);
  Serial.println("[SENSORES] Pines configurados");
  Serial.println("[THERMO] GPIO4 (simulado como entrada analógica)");
  Serial.println("[AIRFLOW] GPIO5 (potenciómetro)");

  // Initialize CAN
  setupCAN();

  Serial.println("=== SISTEMA LISTO ===");
}

/* ================================================================ */
/*                    MAIN LOOP                                      */
/* ================================================================ */

void loop() {
  unsigned long now = millis();

  // --- 1. Read sensors and transmit via CAN every 30 seconds ---
  if (now - lastSensorRead >= INTERVAL_READ_SEND) {
    lastSensorRead = now;
    readAllSensors();
    transmitSensorData();
    printSerialDiagnostics();
  }

  // --- 2. Check for incoming CAN messages ---
  receiveCANMessages();

  // --- 3. Periodic CAN status ---
  if (now - lastCanStatus >= INTERVAL_CAN_STATUS) {
    lastCanStatus = now;
    printCANStatus();
  }
}
