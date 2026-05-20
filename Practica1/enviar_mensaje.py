import serial
import time
import serial.tools.list_ports

# Función para listar puertos disponibles
def listar_puertos():
    print("Buscando puertos...")
    ports = serial.tools.list_ports.comports()
    result = []
    for port in ports:
        print(f"Encontrado: {port.device} - {port.description}")
        result.append(port.device)
    if not result:
        print("No se encontraron puertos COM.")
    return result

# Configuración del puerto serie
available_ports = listar_puertos()
if available_ports:
    # Usar el primero encontrado si existe, o mantener COM3 por defecto si quieres forzarlo
    # Para este ejemplo, intentaremos usar el primero si COM3 no está
    if "COM3" in available_ports:
        PUERTO_COM = "COM3"
    else:
        print(f"COM3 no encontrado. Usando {available_ports[0]} en su lugar.")
        PUERTO_COM = available_ports[0]
else:
    PUERTO_COM = "COM3" # Fallback para que intente y muestre el error original si no hay nada

BAUDIOS = 9600

try:
    print(f"Intentando conectar a {PUERTO_COM}...")
    # Abrir el puerto serie
    uart = serial.Serial(
        port=PUERTO_COM,
        baudrate=BAUDIOS,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        timeout=1
    )

    time.sleep(2)  # Tiempo de inicialización
    print("Puerto serie abierto correctamente")

    # Envío de datos
    while True:
        dato = input("Introduce el carácter a enviar (q para salir): ")

        if dato.lower() == 'q':
            break

        uart.write(dato.encode('ascii'))
        print("Dato enviado")

except serial.SerialException as error:
    print("Error en la comunicación serie:", error)

finally:
    if 'uart' in locals() and uart.is_open:
        uart.close()
        print("Puerto serie cerrado")
