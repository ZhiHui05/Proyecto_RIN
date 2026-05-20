import serial.tools.list_ports

def listar_puertos():
    puertos = serial.tools.list_ports.comports()
    if not puertos:
        print("No se encontraron puertos COM disponibles.")
    else:
        print("Puertos COM disponibles:")
        for puerto in puertos:
            print(f"- {puerto.device}: {puerto.description}")

if __name__ == "__main__":
    listar_puertos()
