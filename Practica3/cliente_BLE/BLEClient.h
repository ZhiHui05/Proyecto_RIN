#ifndef BLE_CLIENT_H
#define BLE_CLIENT_H

#include <Arduino.h>
#include <BLEDevice.h>

/**
 * @brief Clase para gestionar la conexión y comunicación de un Cliente BLE.
 * Encapsula la inicialización, la búsqueda de dispositivos y la lectura/escritura de características.
 */
class BLEClientManager {
public:
    /**
     * @brief Constructor del gestor BLE.
     * @param serviceUUID UUID del servicio al que nos queremos conectar.
     * @param charUUID1 UUID de la característica 1 (usada para notificaciones).
     * @param charUUID2 UUID de la característica 2 (usada para lectura y escritura).
     */
    BLEClientManager(const char* serviceUUID, const char* charUUID1, const char* charUUID2);
    
    /**
     * @brief Inicializa el dispositivo BLE y comienza a escanear servidores cercanos.
     */
    void begin();
    
    /**
     * @brief Máquina de estados. Debe llamarse en el loop() principal.
     * Gestiona las conexiones pendientes y reinicia el escaneo si es necesario.
     */
    void update();
    
    /**
     * @brief Verifica si actualmente estamos conectados al servidor BLE.
     * @return true si está conectado, false en caso contrario.
     */
    bool isConnected();
    
    /**
     * @brief Lee el valor actual de la segunda característica.
     * @return String con el valor leído.
     */
    String readCharacteristic2();
    
    /**
     * @brief Escribe un nuevo valor en la segunda característica.
     * @param txValue String con el valor a enviar al servidor.
     */
    void writeCharacteristic2(String txValue);

    // Banderas de estado
    bool doConnect;
    bool connected;
    bool doScan;
    
    BLEAdvertisedDevice* myDevice;
    BLERemoteCharacteristic* pRemoteChar_1;
    BLERemoteCharacteristic* pRemoteChar_2;

private:
    BLEUUID serviceUUID;
    BLEUUID charUUID_1;
    BLEUUID charUUID_2;

    bool connectToServer();
    bool connectCharacteristic(BLERemoteService* pRemoteService, BLERemoteCharacteristic* l_BLERemoteChar);
};

// Declaración de callbacks manejados internamente
void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);

#endif // BLE_CLIENT_H
