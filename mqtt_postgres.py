import os
import json
import paho.mqtt.client as mqtt
import psycopg2

# Solución para evitar que errores en español de Windows hagan colapsar a psycopg2
os.environ["PGCLIENTENCODING"] = "utf-8"

# -----------------------------------
# CONEXION POSTGRESQL
# -----------------------------------

try:
    # Intenta conectarse a la base de datos PostgreSQL local
    conexion = psycopg2.connect(
        host="localhost",
        database="ESP32",
        user="postgres",
        password="postgres"
    )
except UnicodeDecodeError as e:
    # Si falla mostrando un error con caracteres especiales (error común en Windows)
    print("Error de conexión a PostgreSQL. Por favor, verifica que:")
    print("1. El servidor PostgreSQL está encendido.")
    print("2. La base de datos 'ESP32' existe.")
    print("3. El usuario y contraseña son correctos.")
    print(f"Detalle técnico del error de codificación: {e}")
    exit(1)
except Exception as e:
    # Atrapa cualquier otro error de conexión y detiene el script
    print(f"Error al conectar a la base de datos: {e}")
    exit(1)

# Crea un 'cursor' para poder ejecutar comandos SQL dentro de la base de datos
cursor = conexion.cursor()

# -----------------------------------
# CUANDO LLEGA MENSAJE MQTT
# -----------------------------------

# Función que se ejecuta automáticamente cuando llega un mensaje de MQTT
def on_message(client, userdata, msg):

    topic = msg.topic 
    # Convierte el contenido del mensaje de bytes a formato texto (string)
    payload = msg.payload.decode()

    print(f"Topic: {topic}")
    print(f"Payload: {payload}")

    # Convierte el texto (formato JSON) en un diccionario de variables en Python
    data = json.loads(payload)

    # -----------------------------------
    # SENSOR DHT11
    # -----------------------------------

    # Revisa si el mensaje vino del tema (topic) del sensor DHT11
    if topic == "sensores/dht11":

        # Extrae los valores del diccionario
        temperatura = data["temperatura"]
        humedad = data["humedad"]
        estado = data["estado"]

        # Ejecuta código SQL para insertar (guardar) los datos en la tabla 'sensordht11'
        cursor.execute(
            """
            INSERT INTO sensordht11
            (temperatura, humedad, estado)
            VALUES (%s, %s, %s)
            """,
            (temperatura, humedad, estado)
        )

        # Confirma (aplica) los cambios en la base de datos
        conexion.commit()

        print("DHT11 guardado")

    # -----------------------------------
    # SENSOR HW-038
    # -----------------------------------

    # Revisa si el mensaje vino del tema (topic) del sensor HW-038
    elif topic == "sensores/hw038":

        # Extrae los valores del diccionario
        nivel = data["nivel"]
        valor_analogico = data["valor_analogico"]

        # Ejecuta código SQL para insertar los datos en la tabla 'sensor_hw038'
        cursor.execute(
            """
            INSERT INTO sensor_hw038
            (nivel, valor_analogico)
            VALUES (%s, %s)
            """,
            (nivel, valor_analogico)
        )

        # Confirma (aplica) los cambios en la base de datos
        conexion.commit()

        print("HW038 guardado")

# -----------------------------------
# MQTT CLIENTE
# -----------------------------------

# Creamos una instancia del cliente para usar MQTT
cliente = mqtt.Client()

# Le indicamos al cliente MQTT qué función usar cuando reciba un mensaje
cliente.on_message = on_message

# Nos conectamos al 'broker' MQTT que corre en la misma computadora (localhost) en el puerto por defecto (1883)
cliente.connect("localhost", 1883, 60)

# Nos suscribimos a los temas (topics) específicos en los cuales van a publicar los ESP32
cliente.subscribe("sensores/dht11")
cliente.subscribe("sensores/hw038")

print("Esperando mensajes MQTT...")

# Inicia un ciclo infinito en el que el programa se queda 'escuchando' indefinidamente sin cerrarse
cliente.loop_forever()