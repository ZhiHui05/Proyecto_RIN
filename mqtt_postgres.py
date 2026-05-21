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
    # ZONA FERMENTACIÓN
    # -----------------------------------

    # Revisa si el mensaje vino del tema (topic) de la zona de fermentación
    if topic == "zonas/fermentacion":

        # Extrae los valores del diccionario
        temperatura = data["temperatura"]
        humedad = data["humedad"]
        co2 = data["co2"]

        # Ejecuta código SQL para insertar (guardar) los datos en la tabla 'zona_fermentacion'
        cursor.execute(
            """
            INSERT INTO zona_fermentacion
            (temperatura, humedad, co2)
            VALUES (%s, %s, %s)
            """,
            (temperatura, humedad, co2)
        )

        # Confirma (aplica) los cambios en la base de datos
        conexion.commit()

        print("Zona Fermentación guardada")

    # -----------------------------------
    # ZONA HORNEADO
    # -----------------------------------

    # Revisa si el mensaje vino del tema (topic) de la zona de horneado
    elif topic == "zonas/horneado":

        # Extrae los valores del diccionario
        temperatura = data["temperatura"]
        flujo_aire = data["flujo_aire"]

        # Ejecuta código SQL para insertar los datos en la tabla 'zona_horneado'
        cursor.execute(
            """
            INSERT INTO zona_horneado
            (temperatura, flujo_aire)
            VALUES (%s, %s)
            """,
            (temperatura, flujo_aire)
        )

        # Confirma (aplica) los cambios en la base de datos
        conexion.commit()

        print("Zona Horneado guardada")

    # -----------------------------------
    # ZONA MATERIAS PRIMAS
    # -----------------------------------

    # Revisa si el mensaje vino del tema (topic) de la zona de materias primas
    elif topic == "zonas/materias":

        # Extrae los valores del diccionario
        temperatura = data["temperatura"]
        humedad = data["humedad"]
        luminosidad = data["luminosidad"]
        nivel_agua = data["nivel_agua"]
        valor_analogico_agua = data["valor_analogico_agua"]

        # Ejecuta código SQL para insertar los datos en la tabla 'zona_materias_primas'
        cursor.execute(
            """
            INSERT INTO zona_materias_primas
            (temperatura, humedad, luminosidad, nivel_agua, valor_analogico_agua)
            VALUES (%s, %s, %s, %s, %s)
            """,
            (temperatura, humedad, luminosidad, nivel_agua, valor_analogico_agua)
        )

        # Confirma (aplica) los cambios en la base de datos
        conexion.commit()

        print("Zona Materias Primas guardada")

# -----------------------------------
# MQTT CLIENTE
# -----------------------------------

# Creamos una instancia del cliente para usar MQTT
cliente = mqtt.Client()

# Le indicamos al cliente MQTT qué función usar cuando reciba un mensaje
cliente.on_message = on_message

# Nos conectamos al 'broker' MQTT externo
cliente.connect("broker.mqtt-dashboard.com", 1883, 60)

# Nos suscribimos a los temas (topics) específicos en los cuales van a publicar los ESP32
cliente.subscribe("zonas/fermentacion")
cliente.subscribe("zonas/horneado")
cliente.subscribe("zonas/materias")

print("Esperando mensajes MQTT...")

# Inicia un ciclo infinito en el que el programa se queda 'escuchando' indefinidamente sin cerrarse
cliente.loop_forever()