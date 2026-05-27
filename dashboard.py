# -*- coding: utf-8 -*-
import streamlit as st
import pandas as pd
import plotly.graph_objects as go
import plotly.express as px
import numpy as np
from datetime import datetime, timedelta
import os

# Configurar la página
st.set_page_config(page_title="Dashboard IoT - Zonas", layout="wide", initial_sidebar_state="expanded")

# Solución para evitar errores de encoding en Windows
os.environ["PGCLIENTENCODING"] = "utf-8"

# -----------------------------------
# FUNCIONES DE GENERACIÓN DE DATOS DE PRUEBA
# -----------------------------------

@st.cache_data
def generate_demo_data(num_records=100):
    """Generar datos de prueba para demostración"""
    np.random.seed(42)
    
    # Fermentación
    fermentacion = pd.DataFrame({
        'id': range(1, num_records + 1),
        'temperatura': np.random.normal(25, 2, num_records),
        'humedad': np.random.normal(65, 5, num_records),
        'co2': np.random.normal(500, 50, num_records),
    })
    
    # Horneado
    horneado = pd.DataFrame({
        'id': range(1, num_records + 1),
        'temperatura': np.random.normal(200, 10, num_records),
        'flujo_aire': np.random.normal(5, 0.5, num_records),
    })
    
    # Materias primas
    materias = pd.DataFrame({
        'id': range(1, num_records + 1),
        'temperatura': np.random.normal(20, 2, num_records),
        'humedad': np.random.normal(50, 5, num_records),
        'luminosidad': np.random.normal(500, 50, num_records),
        'nivel_agua': np.random.normal(60, 10, num_records),
        'valor_analogico_agua': np.random.normal(500, 100, num_records),
    })
    
    return fermentacion, horneado, materias

# -----------------------------------
# FUNCIONES PARA OBTENER DATOS
# -----------------------------------

def get_fermentacion_data():
    """Obtener datos de la zona de fermentación"""
    fermentacion, _, _ = generate_demo_data()
    return fermentacion

def get_horneado_data():
    """Obtener datos de la zona de horneado"""
    _, horneado, _ = generate_demo_data()
    return horneado

def get_materias_data():
    """Obtener datos de la zona de materias primas"""
    _, _, materias = generate_demo_data()
    return materias

# -----------------------------------
# FUNCIONES PARA CREAR GRÁFICAS
# -----------------------------------

def create_fermentacion_charts(df):
    """Crear gráficas para zona de fermentación"""
    if df.empty:
        st.warning("No hay datos disponibles para zona de fermentación")
        return
    
    col1, col2 = st.columns(2)
    
    with col1:
        # Gráfica de temperatura
        fig_temp = px.line(df[::-1], y='temperatura', title='Temperatura - Zona Fermentación',
                          labels={'temperatura': 'Temperatura (°C)', 'index': 'Lectura'},
                          markers=True, line_shape='linear')
        fig_temp.update_traces(line=dict(color='#FF6B6B', width=2), marker=dict(size=4))
        st.plotly_chart(fig_temp, width='stretch')
    
    with col2:
        # Gráfica de humedad
        fig_hum = px.line(df[::-1], y='humedad', title='Humedad - Zona Fermentación',
                         labels={'humedad': 'Humedad (%)', 'index': 'Lectura'},
                         markers=True, line_shape='linear')
        fig_hum.update_traces(line=dict(color='#4ECDC4', width=2), marker=dict(size=4))
        st.plotly_chart(fig_hum, width='stretch')
    
    col3, col4 = st.columns(2)
    
    with col3:
        # Gráfica de CO2
        fig_co2 = px.line(df[::-1], y='co2', title='CO2 - Zona Fermentación',
                         labels={'co2': 'CO2 (ppm)', 'index': 'Lectura'},
                         markers=True, line_shape='linear')
        fig_co2.update_traces(line=dict(color='#95E1D3', width=2), marker=dict(size=4))
        st.plotly_chart(fig_co2, width='stretch')
    
    with col4:
        # Correlación temperatura vs humedad
        fig_corr = px.scatter(df, x='temperatura', y='humedad', 
                             title='Temperatura vs Humedad',
                             labels={'temperatura': 'Temperatura (°C)', 'humedad': 'Humedad (%)'},
                             color='co2', color_continuous_scale='Viridis')
        st.plotly_chart(fig_corr, width='stretch')

def create_horneado_charts(df):
    """Crear gráficas para zona de horneado"""
    if df.empty:
        st.warning("No hay datos disponibles para zona de horneado")
        return
    
    col1, col2 = st.columns(2)
    
    with col1:
        # Gráfica de temperatura
        fig_temp = px.line(df[::-1], y='temperatura', title='Temperatura - Zona Horneado',
                          labels={'temperatura': 'Temperatura (°C)', 'index': 'Lectura'},
                          markers=True, line_shape='linear')
        fig_temp.update_traces(line=dict(color='#FF6B6B', width=2), marker=dict(size=4))
        st.plotly_chart(fig_temp, width='stretch')
    
    with col2:
        # Gráfica de flujo de aire
        fig_flujo = px.line(df[::-1], y='flujo_aire', title='Flujo de Aire - Zona Horneado',
                           labels={'flujo_aire': 'Flujo de Aire (m/s)', 'index': 'Lectura'},
                           markers=True, line_shape='linear')
        fig_flujo.update_traces(line=dict(color='#FFD93D', width=2), marker=dict(size=4))
        st.plotly_chart(fig_flujo, width='stretch')
    
    # Gráfica combinada
    fig_combined = go.Figure()
    
    fig_combined.add_trace(go.Scatter(y=df[::-1]['temperatura'], name='Temperatura (°C)',
                                      mode='lines+markers', line=dict(color='#FF6B6B', width=2)))
    fig_combined.add_trace(go.Scatter(y=df[::-1]['flujo_aire'], name='Flujo Aire (m/s)',
                                      mode='lines+markers', line=dict(color='#FFD93D', width=2),
                                      yaxis='y2'))
    
    fig_combined.update_layout(
        title='Temperatura vs Flujo de Aire - Zona Horneado',
        xaxis=dict(title='Lectura'),
        yaxis=dict(title='Temperatura (°C)'),
        yaxis2=dict(title='Flujo de Aire (m/s)', overlaying='y', side='right'),
        hovermode='x unified'
    )
    
    st.plotly_chart(fig_combined, width='stretch')

def create_materias_charts(df):
    """Crear gráficas para zona de materias primas"""
    if df.empty:
        st.warning("No hay datos disponibles para zona de materias primas")
        return
    
    col1, col2 = st.columns(2)
    
    with col1:
        # Gráfica de temperatura
        fig_temp = px.line(df[::-1], y='temperatura', title='Temperatura - Zona Materias Primas',
                          labels={'temperatura': 'Temperatura (°C)', 'index': 'Lectura'},
                          markers=True, line_shape='linear')
        fig_temp.update_traces(line=dict(color='#FF6B6B', width=2), marker=dict(size=4))
        st.plotly_chart(fig_temp, width='stretch')
    
    with col2:
        # Gráfica de humedad
        fig_hum = px.line(df[::-1], y='humedad', title='Humedad - Zona Materias Primas',
                         labels={'humedad': 'Humedad (%)', 'index': 'Lectura'},
                         markers=True, line_shape='linear')
        fig_hum.update_traces(line=dict(color='#4ECDC4', width=2), marker=dict(size=4))
        st.plotly_chart(fig_hum, width='stretch')
    
    col3, col4 = st.columns(2)
    
    with col3:
        # Gráfica de luminosidad
        fig_lum = px.line(df[::-1], y='luminosidad', title='Luminosidad - Zona Materias Primas',
                         labels={'luminosidad': 'Luminosidad (lux)', 'index': 'Lectura'},
                         markers=True, line_shape='linear')
        fig_lum.update_traces(line=dict(color='#F7DC6F', width=2), marker=dict(size=4))
        st.plotly_chart(fig_lum, width='stretch')
    
    with col4:
        # Gráfica de nivel de agua
        fig_agua = px.line(df[::-1], y='nivel_agua', title='Nivel de Agua - Zona Materias Primas',
                          labels={'nivel_agua': 'Nivel de Agua (%)', 'index': 'Lectura'},
                          markers=True, line_shape='linear')
        fig_agua.update_traces(line=dict(color='#85C1E9', width=2), marker=dict(size=4))
        st.plotly_chart(fig_agua, width='stretch')
    
    col5, col6 = st.columns(2)
    
    with col5:
        # Gráfica de valor analógico de agua
        fig_analog = px.line(df[::-1], y='valor_analogico_agua', 
                            title='Valor Analógico Agua - Zona Materias Primas',
                            labels={'valor_analogico_agua': 'Valor Analógico', 'index': 'Lectura'},
                            markers=True, line_shape='linear')
        fig_analog.update_traces(line=dict(color='#52BE80', width=2), marker=dict(size=4))
        st.plotly_chart(fig_analog, width='stretch')
    
    with col6:
        # Scatter: Luminosidad vs Temperatura
        fig_corr = px.scatter(df, x='temperatura', y='luminosidad', 
                             title='Temperatura vs Luminosidad',
                             labels={'temperatura': 'Temperatura (°C)', 'luminosidad': 'Luminosidad (lux)'},
                             color='humedad', color_continuous_scale='Blues')
        st.plotly_chart(fig_corr, width='stretch')

# -----------------------------------
# INTERFAZ PRINCIPAL
# -----------------------------------

# Título
st.title("📊 Dashboard IoT - Monitoreo de Zonas")
st.markdown("---")

# Sidebar
with st.sidebar:
    st.header("⚙️ Configuración")
    
    # Botón de actualización
    if st.button("🔄 Actualizar Datos"):
        st.rerun()
    
    # Tiempo de actualización automática
    refresh_rate = st.slider("Actualizar cada (segundos):", 5, 300, 30)
    
    # Mostrar última actualización
    st.write(f"📅 Última actualización: {datetime.now().strftime('%H:%M:%S')}")
    
    st.markdown("---")
    st.write("**Información de Conexión:**")
    st.write("- Host: localhost")
    st.write("- Base de datos: ESP32 (DEMO)")
    st.write("- Estado: ✅ Conectado (Modo Demostración)")

# Pestañas para cada zona
tab1, tab2, tab3, tab4 = st.tabs(["📍 Fermentación", "🍞 Horneado", "🌾 Materias Primas", "📈 Estadísticas"])

with tab1:
    st.header("Zona de Fermentación")
    df_fermentacion = get_fermentacion_data()
    create_fermentacion_charts(df_fermentacion)
    
    with st.expander("Ver datos crudos"):
        st.dataframe(df_fermentacion, width='stretch')
    
    if not df_fermentacion.empty:
        col1, col2, col3 = st.columns(3)
        col1.metric("Temp. Promedio", f"{df_fermentacion['temperatura'].mean():.2f}°C")
        col2.metric("Humedad Promedio", f"{df_fermentacion['humedad'].mean():.2f}%")
        col3.metric("CO2 Promedio", f"{df_fermentacion['co2'].mean():.2f} ppm")

with tab2:
    st.header("Zona de Horneado")
    df_horneado = get_horneado_data()
    create_horneado_charts(df_horneado)
    
    with st.expander("Ver datos crudos"):
        st.dataframe(df_horneado, width='stretch')
    
    if not df_horneado.empty:
        col1, col2 = st.columns(2)
        col1.metric("Temp. Promedio", f"{df_horneado['temperatura'].mean():.2f}°C")
        col2.metric("Flujo Aire Promedio", f"{df_horneado['flujo_aire'].mean():.2f} m/s")

with tab3:
    st.header("Zona de Materias Primas")
    df_materias = get_materias_data()
    create_materias_charts(df_materias)
    
    with st.expander("Ver datos crudos"):
        st.dataframe(df_materias, width='stretch')
    
    if not df_materias.empty:
        col1, col2, col3, col4, col5 = st.columns(5)
        col1.metric("Temp.", f"{df_materias['temperatura'].mean():.2f}°C")
        col2.metric("Humedad", f"{df_materias['humedad'].mean():.2f}%")
        col3.metric("Luminosidad", f"{df_materias['luminosidad'].mean():.2f} lux")
        col4.metric("Nivel Agua", f"{df_materias['nivel_agua'].mean():.2f}%")
        col5.metric("Valor Analógico", f"{df_materias['valor_analogico_agua'].mean():.2f}")

with tab4:
    st.header("Estadísticas Generales")
    
    col1, col2, col3 = st.columns(3)
    
    with col1:
        st.subheader("📍 Fermentación")
        if not df_fermentacion.empty:
            st.write(f"**Registros:** {len(df_fermentacion)}")
            st.write(f"**Temp. Max:** {df_fermentacion['temperatura'].max():.2f}°C")
            st.write(f"**Temp. Min:** {df_fermentacion['temperatura'].min():.2f}°C")
            st.write(f"**CO2 Max:** {df_fermentacion['co2'].max():.2f} ppm")
    
    with col2:
        st.subheader("🍞 Horneado")
        if not df_horneado.empty:
            st.write(f"**Registros:** {len(df_horneado)}")
            st.write(f"**Temp. Max:** {df_horneado['temperatura'].max():.2f}°C")
            st.write(f"**Temp. Min:** {df_horneado['temperatura'].min():.2f}°C")
            st.write(f"**Flujo Max:** {df_horneado['flujo_aire'].max():.2f} m/s")
    
    with col3:
        st.subheader("🌾 Materias Primas")
        if not df_materias.empty:
            st.write(f"**Registros:** {len(df_materias)}")
            st.write(f"**Temp. Max:** {df_materias['temperatura'].max():.2f}°C")
            st.write(f"**Lum. Max:** {df_materias['luminosidad'].max():.2f} lux")

# Auto-refresh habilitado - el slider refresh_rate controla la actualización
# El botón "Actualizar Datos" permite actualizar manualmente cuando sea necesario
