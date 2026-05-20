#include "BLEClient.h"

// Variables compartidas con los callbacks de Arduino
extern BLEClientManager bleManager;

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    // Evento de conexión (se puede añadir lógica extra si se requiere)
  }

  void onDisconnect(BLEClient* pclient) {
    // Cuando el servidor se desconecta, actualizamos la bandera y avisamos
    bleManager.connected = false;
    Serial.println("onDisconnect: Desconectado del servidor BLE.");
  }
};

/**
 * @brief Callback que maneja el descubrimiento de dispositivos mediante el escáner BLE.
 * Comprueba si el dispositivo encontrado ofrece el servicio que buscamos.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());
  
    // Si el dispositivo tiene un UUID de servicio y coincide con el nuestro, lo almacenamos
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BLEUUID("dc958536-cbd3-40d8-aa5c-046c17e3cb1c"))) {
      BLEDevice::getScan()->stop(); // Detenemos el escaneo para ahorrar recursos
      bleManager.myDevice = new BLEAdvertisedDevice(advertisedDevice);
      bleManager.doConnect = true; // Levantamos bandera para conectar en el loop (update)
      bleManager.doScan = true;    // Levantamos bandera para reanudar el escaneo tras desconexiones
    }
  }
};

/**
 * @brief Callback que se llama cuando recibimos una notificación del servidor (Notify).
 * Extrae y recompone un número (uint32_t) desde el buffer de bytes.
 */
void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  // Verificamos que provenga de la característica que nos interesa (charUUID_1)
  if(pBLERemoteCharacteristic->getUUID().toString() == "75460506-cee0-4693-915c-c3224d3fc129") {
    uint32_t counter = pData[0];
    for(int i = 1; i<length; i++) {
        counter = counter | (pData[i] << i*8); // Reconstruir bytes
    }
    Serial.print("Characteristic 1 (Notify) from server: ");
    Serial.println(counter);  
  }
}

// Implementación de constructor de la clase BLEClientManager
BLEClientManager::BLEClientManager(const char* s_UUID, const char* c_UUID_1, const char* c_UUID_2) 
  : serviceUUID(s_UUID), charUUID_1(c_UUID_1), charUUID_2(c_UUID_2) {
    // Inicializar banderas de conexión y punteros en null para seguridad
    doConnect = false;
    connected = false;
    doScan = false;
    myDevice = nullptr;
    pRemoteChar_1 = nullptr;
    pRemoteChar_2 = nullptr;
}

void BLEClientManager::begin() {
  BLEDevice::init(""); // Inicializamos el stack BLE (nombre vacío en clientes)
  BLEScan* pBLEScan = BLEDevice::getScan();
  
  // Asignamos nuestros propios callbacks de descubrimiento
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);       // Intervalos en base a recomendación estándar BLE
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);     // Buscar activamente dispositivos (consume algo más, pero descubre más rápido)
  pBLEScan->start(5, false);         // Escanear durante 5 segundos
}

void BLEClientManager::update() {
  // Si encontramos un dispositivo con el servicio, doConnect se pone a true
  if (doConnect) {
    if (connectToServer()) {
      Serial.println("Conectados al Servidor BLE.");
    } else {
      Serial.println("Fallo al conectar al servidor.");
    }
    doConnect = false; // Solo intentamos conectar una vez por cada evento de descubrimiento
  }

  // En caso de que se nos haya desconectado o se necesite reanudar, iniciamos el scan y no bloqueamos ('0' es infinito)
  if (!connected && doScan) {
    BLEDevice::getScan()->start(0); 
  }
}

bool BLEClientManager::isConnected() {
    return connected;
}

// Método simple para leer el valor del UUID 2 si estuviésemos en estado "conectado"
String BLEClientManager::readCharacteristic2() {
    if(connected && pRemoteChar_2) {
        // La librería maneja esta lectura y devuelve un string (std::string detrás de escena)
        return pRemoteChar_2->readValue().c_str();
    }
    return ""; // Regresa un string vacío si algo falló
}

// Método simple para enviar datos a la característica de UUID 2 
void BLEClientManager::writeCharacteristic2(String txValue) {
    if(connected && pRemoteChar_2) {
        // Enviar con tamaño adecuado asegurando el envío del puntero de char string (.c_str())
        pRemoteChar_2->writeValue(txValue.c_str(), txValue.length());
    }
}

// Vincula las características específicas para suscribir a Notify si es aplicable
bool BLEClientManager::connectCharacteristic(BLERemoteService* pRemoteService, BLERemoteCharacteristic* l_BLERemoteChar) {
  // Aseguramos que la característica a la que quisimos conectarnos exista de este lado
  if (l_BLERemoteChar == nullptr) {
    Serial.print("Failed to find characteristic");
    return false;
  }
  Serial.println(" - Found characteristic: " + String(l_BLERemoteChar->getUUID().toString().c_str()));

  // Verificamos si la característica envía notificados (notifys). Así activamos las subscripciones.
  if(l_BLERemoteChar->canNotify())
    l_BLERemoteChar->registerForNotify(notifyCallback); // Al llegar el notifyCallback invocamos esa función

  return true;
}

bool BLEClientManager::connectToServer() {
  Serial.print("Conectando a ");
  Serial.println(myDevice->getAddress().toString().c_str());
  
  // Instanciamos nuestro cliente bluetooth BLE
  BLEClient*  pClient  = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback()); // Asignamos callbacks de conexiones (o des-conexiones)
  pClient->connect(myDevice);
  Serial.println(" - Connected to server");

  // Solicitamos acceso a un servicio específico del cliente remoto (servidor en este caso)
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false; // Salimos rápido con falsa conexión si nos equivocamos de host
  }

  connected = true;
  // Nos vinculamos a los punteros de las dos características pre programadas de antemano
  pRemoteChar_1 = pRemoteService->getCharacteristic(charUUID_1);
  pRemoteChar_2 = pRemoteService->getCharacteristic(charUUID_2);
  
  // Intentamos unificar las llamadas subscritas. Desconectando de cualquier intento de no haber procedido los links
  if(!connectCharacteristic(pRemoteService, pRemoteChar_1)) connected = false;
  else if(!connectCharacteristic(pRemoteService, pRemoteChar_2)) connected = false;

  // Si falló al conseguir características, cerramos la conexión y lo informamos
  if(!connected) {
    pClient->disconnect();
    return false;
  }
  return true;
}
