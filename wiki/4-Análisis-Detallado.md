# Análisis Detallado por Dispositivo

## Laptop A: Alejandro (AMD Ryzen 5 4500U)

| Configuración | Tiempo | Nota |
|---|---|---|
| 6 hilos | 10.5160s | Sin hiperthreading: máximo teórico |
| 12 hilos | 9.8120s | OpenMP sobre-suscribe a 2 hilos/núcleo |
| 18 hilos | 9.6230s | ⭐ Mejor: mayor granularidad en tareas |

### 📌 Interpretación

**Limitación física:** El Ryzen 5 4500U tiene exactamente **6 núcleos sin hyperthreading**. Pedir 12 o 18 hilos obliga al planificador a repartir hilos entre núcleos reales.

**Comportamiento observado:**
- A 6 hilos: Un hilo por núcleo (eficiente pero no escala bien)
- A 12/18 hilos: OpenMP crea más hilos, lo que permite al OS alternar entre ellos mientras unos esperan I/O → mejora percibida

**Por qué 18 > 12:** Con 18 tareas independientes y 24 secciones OpenMP, la granularidad más fina permite mejor distribución de carga.

### ⚠️ Conclusión para Laptop A
- Rendimiento: **Bajo (peor del equipo)**
- Escalabilidad: **Limitada por hardware**
- Recomendación: **Nodo secundario de apoyo**
- Asignar: Lotes pequeños (1-2 imágenes) para evitar retrasos

---

## Laptop B: Francisco (Intel Core i5-1135G7 + Linux Bodhi)

| Configuración | Tiempo | Nota |
|---|---|---|
| 6 hilos | 7.3627s | ⭐ Mejor entre todos los "6 hilos" |
| 12 hilos | 7.1117s | Reduce 3.4% |
| 18 hilos | **6.8342s** | ⭐⭐ **MEJOR GLOBAL** |

### 📌 Interpretación

**Ventaja multiplicadora:** Linux Bodhi + Intel Gen 11 = combinación óptima

**Factores de éxito:**
1. **Linux Bodhi:** Planificador de procesos más ligero (menos overhead)
2. **I/O eficiente:** Mejor caché y manejo de memoria que Windows
3. **Hilos lógicos (8):** Suficientes para 18 hilos con buena distribución

**Por qué Linux es más rápido:**
- Windows 11: Carga de sistema operativo, servicios en background (Windows Update, antivirus)
- Linux Bodhi: Kernel mínimo, sin overhead innecesario

**Escalabilidad consistente:**
```
Mejora por incremento:
6→12: -3.4% (0.2510s de ganancia)
12→18: -3.9% (0.2775s de ganancia)
Tendencia: lineal y predecible
```

### ✅ Conclusión para Laptop B
- Rendimiento: **Excelente (mejor del equipo)**
- Escalabilidad: **Lineal y predecible**
- Estabilidad: **Alta, sin anomalías**
- Recomendación: **NODO MAESTRO**
- Función: Distribuidor principal y coordinador

---

## Laptop C: Yahel (Intel Core i5-1235U)

| Configuración | Tiempo | Nota |
|---|---|---|
| 6 hilos | 7.7310s | Buen inicio |
| 12 hilos | 8.2830s | ⚠️ **Regresión (+7.1%)** |
| 18 hilos | 6.9550s | ⭐ Recuperación, casi igual a Laptop B |

### 📌 Interpretación

**Especificaciones más altas, pero comportamiento errático:**

La Laptop C tiene:
- ✅ 12 hilos lógicos (más que B y C)
- ✅ 10 núcleos físicos (P-cores + E-cores)
- ✅ Generación más reciente (12va Intel)

**¿Por qué regresa a 12 hilos?**

**Causa probable: Thermal Throttling**
```
Intel Core i5-1235U:
- Frecuencia base: 1.3 GHz
- Frecuencia máxima: 4.4 GHz (con turbo)
- A 12 hilos: CPU alcanza ~85°C → thermal throttling
- A 18 hilos: Distribución mejor, menos hot-spot termal
```

**Alternativa:** Tareas en segundo plano de Windows 11 (Defender, update, etc.)

### 📊 Análisis P-cores vs E-cores

La CPU tiene arquitectura híbrida:
- **P-cores (Performance):** 2 núcleos, ejecutan código crítico
- **E-cores (Efficiency):** 8 núcleos, carga ligera

A 12 hilos: OS podría estar saturando solo los P-cores → cuello botella  
A 18 hilos: Distribuye mejor entre P y E-cores → mejor rendimiento

### ⚠️ Conclusión para Laptop C
- Rendimiento: **Bueno (similar a B en su mejor momento)**
- Escalabilidad: **Inestable en 12 hilos**
- Estabilidad: **Media (afectada por Windows 11)**
- Recomendación: **NODO SECUNDARIO DE ALTO RENDIMIENTO**
- Función: Apoyo principal (con 18 hilos configurado)
- **Importante:** Mantener a 18 hilos para evitar throttling

---

## 🔬 Factor Sistema Operativo

### Comparativa Windows vs Linux

**Linux Bodhi (Laptop B):** 6.8342s  
**Windows 11 (Laptops A+C promedio):** 8.4070s

**Diferencia: 22.9% más lento en Windows**

### Razones técnicas

| Aspecto | Linux | Windows |
|---|---|---|
| **Overhead kernel** | Bajo | Medio-Alto |
| **Servicios background** | Mínimos | Windows Update, Defender, etc. |
| **Planificador I/O** | Optimizado | Más latencia |
| **Context switching** | Rápido | Más overhead |
| **Virtualización** | No (Bodhi) | Posible Hyper-V activo |

### Impacto en código I/O-bound

El código es **I/O bound** (limitado por operaciones de disco):
- Lectura de BMP
- Procesamiento (paralelizado)
- Escritura de resultados

**En Linux:** Mientras hilos esperan I/O, otro hilo puede ejecutarse sin overhead  
**En Windows:** Mayor latencia en context switching

---

## 📍 Punto óptimo de hilos

### ¿Por qué 18 fue mejor para todos?

El código genera **24 tareas independientes** (6 filtros × 4 imágenes).

```
A 6 hilos:  Cada hilo ejecuta 4 tareas pesadas
A 12 hilos: Cada hilo ejecuta 2 tareas
A 18 hilos: Cada hilo ejecuta 1-2 tareas (granularidad ideal)
A 24 hilos: Sería máximo teórico, pero hay overhead de sincronización
```

**Conclusión:** 18 hilos equilibra:
- ✅ Granularidad: Tareas lo bastante pequeñas
- ✅ Overhead: Sin sincronización excesiva
- ✅ Caché: Eficiente en reutilización
- ✅ I/O: Máxima oportunidad de alternancia

---

[[3-Resultados-y-Datos]] | [[Home]] | [[5-Conclusiones]]
