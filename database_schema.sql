-- ============================================
-- SCHEMA DE BASE DE DATOS PARA RIN
-- ============================================

-- ============================================
-- TABLA: zona_fermentacion
-- Almacena datos de temperatura, humedad y CO2
-- Publicado por: sensores/fermentation
-- ============================================

DROP TABLE IF EXISTS zona_fermentacion CASCADE;

CREATE TABLE zona_fermentacion
(
    id SERIAL PRIMARY KEY,
    temperatura DOUBLE PRECISION,
    humedad DOUBLE PRECISION,
    co2 DOUBLE PRECISION,
    fecha TIMESTAMP WITHOUT TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

ALTER TABLE zona_fermentacion OWNER to postgres;

-- Crear índice en fecha para búsquedas rápidas
CREATE INDEX idx_fermentacion_fecha ON zona_fermentacion(fecha);

-- ============================================
-- TABLA: zona_horneado
-- Almacena datos de temperatura y flujo de aire
-- Publicado por: sensores/baking
-- ============================================

DROP TABLE IF EXISTS zona_horneado CASCADE;

CREATE TABLE zona_horneado
(
    id SERIAL PRIMARY KEY,
    temperatura DOUBLE PRECISION,
    flujo_aire DOUBLE PRECISION,
    fecha TIMESTAMP WITHOUT TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

ALTER TABLE zona_horneado OWNER to postgres;

-- Crear índice en fecha para búsquedas rápidas
CREATE INDEX idx_horneado_fecha ON zona_horneado(fecha);

-- ============================================
-- TABLA: zona_materias_primas
-- Almacena datos de sensores de materias primas
-- Publicado por: sensores/raw_materials
-- ============================================

DROP TABLE IF EXISTS zona_materias_primas CASCADE;

CREATE TABLE zona_materias_primas
(
    id SERIAL PRIMARY KEY,
    temperatura DOUBLE PRECISION,
    humedad DOUBLE PRECISION,
    luminosidad DOUBLE PRECISION,
    nivel_agua DOUBLE PRECISION,
    valor_analogico_agua DOUBLE PRECISION,
    fecha TIMESTAMP WITHOUT TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

ALTER TABLE zona_materias_primas OWNER to postgres;

-- Crear índice en fecha para búsquedas rápidas
CREATE INDEX idx_materias_fecha ON zona_materias_primas(fecha);

-- ============================================
-- PROCEDIMIENTO PARA LIMPIAR DATOS ANTIGUOS
-- ============================================

CREATE OR REPLACE FUNCTION limpiar_datos_antiguos()
RETURNS void AS $$
BEGIN
    -- Elimina registros más antiguos de 30 días
    DELETE FROM zona_fermentacion WHERE fecha < NOW() - INTERVAL '30 days';
    DELETE FROM zona_horneado WHERE fecha < NOW() - INTERVAL '30 days';
    DELETE FROM zona_materias_primas WHERE fecha < NOW() - INTERVAL '30 days';
END;
$$ LANGUAGE plpgsql;

-- ============================================
-- VISTAS PARA ANÁLISIS RÁPIDO
-- ============================================

-- Vista: Últimas 24 horas de fermentación
CREATE OR REPLACE VIEW v_fermentacion_24h AS
SELECT * FROM zona_fermentacion 
WHERE fecha >= NOW() - INTERVAL '24 hours'
ORDER BY fecha DESC;

-- Vista: Últimas 24 horas de horneado
CREATE OR REPLACE VIEW v_horneado_24h AS
SELECT * FROM zona_horneado 
WHERE fecha >= NOW() - INTERVAL '24 hours'
ORDER BY fecha DESC;

-- Vista: Últimas 24 horas de materias primas
CREATE OR REPLACE VIEW v_materias_24h AS
SELECT * FROM zona_materias_primas 
WHERE fecha >= NOW() - INTERVAL '24 hours'
ORDER BY fecha DESC;

-- ============================================
-- CONFIRMACIÓN DE CREACIÓN
-- ============================================
-- Las tablas están listas para recibir datos desde MQTT
-- Tópicos esperados:
--   - sensores/fermentation
--   - sensores/baking
--   - sensores/raw_materials
