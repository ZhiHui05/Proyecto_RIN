@echo off
REM Script para ejecutar el Dashboard Streamlit
REM Asegúrate de tener activado el entorno virtual antes de ejecutar este script

cd /d "%~dp0"
streamlit run dashboard.py
pause
