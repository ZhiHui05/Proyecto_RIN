# Dashboard IoT - Zonas de Producción

Dashboard Streamlit para monitorear datos IoT de diferentes zonas de producción (fermentación, horneado y materias primas).

## Características

- 📊 Visualización de datos en tiempo real
- 📍 Monitoreo de Zona de Fermentación (temperatura, humedad, CO2)
- 🍞 Monitoreo de Zona de Horneado (temperatura, flujo de aire)
- 🌾 Monitoreo de Zona de Materias Primas (temperatura, humedad, luminosidad, agua)
- 📈 Estadísticas generales de todas las zonas
- 🔄 Actualización automática de datos

## Requisitos

- Python 3.8+
- Streamlit
- Pandas
- Plotly
- NumPy

## Instalación de dependencias

Las dependencias ya están instaladas. Si necesitas reinstalar:

```bash
pip install streamlit pandas plotly numpy
```

## Uso

### Opción 1: Ejecutar desde PowerShell (RECOMENDADO)

```powershell
# Asegúrate de estar en la carpeta del proyecto
cd d:\Escritorio\RIN

# Si tienes un entorno virtual activado, mantenerlo activado
# Luego ejecuta:
streamlit run dashboard.py
```

### Opción 2: Ejecutar el archivo batch

Haz doble clic en `run_dashboard.bat` en la carpeta del proyecto.

## Modo de Funcionamiento

**Versión Actual: DEMOSTRACIÓN**

El dashboard está configurado en modo demostración con datos generados aleatoriamente. Esto permite:
- Probar la funcionalidad sin dependencias externas
- Verificar que todo funciona correctamente
- Desarrollar nuevas características

Para conectar a una base de datos PostgreSQL real, modifica el archivo `dashboard.py` en la función `generate_demo_data()` o implementa las funciones originales de conexión a base de datos.

## Descripción de Zonas

### 📍 Fermentación
- **Temperatura**: Monitoreo en °C
- **Humedad**: Porcentaje de humedad relativa
- **CO2**: Concentración de dióxido de carbono en ppm

### 🍞 Horneado
- **Temperatura**: Temperatura del horno en °C
- **Flujo de Aire**: Velocidad del flujo de aire en m/s

### 🌾 Materias Primas
- **Temperatura**: Temperatura del almacén en °C
- **Humedad**: Humedad relativa del almacén
- **Luminosidad**: Nivel de luz en lux
- **Nivel de Agua**: Porcentaje del depósito
- **Valor Analógico**: Lectura analógica del sensor de agua

## Solución de Problemas

### Error: "Module not found"
```bash
pip install streamlit pandas plotly numpy
```

### Error de encoding UTF-8
El archivo está configurado con la codificación correcta. Si persisten errores:
- Verifica que tu terminal esté usando UTF-8
- En PowerShell, ejecuta: `[console]::InputEncoding = [console]::OutputEncoding = New-Object System.Text.UTF8Encoding`

### No se abre el navegador
El dashboard estará disponible en: `http://localhost:8501`
Abre esto manualmente en tu navegador.

## Estructura de carpetas

```
RIN/
├── dashboard.py              # Archivo principal
├── run_dashboard.bat         # Script de ejecución
├── README.md                 # Este archivo
└── .streamlit/
    └── config.toml          # Configuración de Streamlit
```

## Notas

- El dashboard se ejecuta en `http://localhost:8501`
- Los datos se actualizan automáticamente según el intervalo configurado en el slider
- Para usar datos reales de una base de datos, conecta PostgreSQL y modifica las funciones de datos

## Autor

Dashboard creado para monitoreo IoT de zonas de producción.
