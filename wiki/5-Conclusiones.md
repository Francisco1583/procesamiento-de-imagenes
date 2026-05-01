# Conclusiones y Recomendaciones

## 🎯 Resumen ejecutivo

Este experimento evalúa qué dispositivo es más adecuado para coordinar un sistema distribuido de procesamiento de imágenes. Los resultados son concluyentes:

**🥇 Nodo Maestro:** Laptop B (Francisco) — 6.8342s @ 18 hilos  
**🥈 Nodo Secundario Premium:** Laptop C (Yahel) — 6.9550s @ 18 hilos  
**🥉 Nodo Secundario Apoyo:** Laptop A (Alejandro) — 9.6230s @ 18 hilos

---

## 1️⃣ Nodo Maestro: Laptop B (Francisco)

### 🏅 Razones técnicas

| Factor | Evaluación |
|---|---|
| **Rendimiento** | ⭐⭐⭐⭐⭐ Mejor tiempo global (6.8342s) |
| **Escalabilidad** | ⭐⭐⭐⭐⭐ Lineal y predecible (7.1-7.4% mejora por incremento) |
| **Estabilidad** | ⭐⭐⭐⭐⭐ Sin anomalías, sin thermal throttling |
| **Consistencia** | ⭐⭐⭐⭐⭐ 3 ejecuciones sin variaciones significativas |
| **Sistema Operativo** | ⭐⭐⭐⭐⭐ Linux Bodhi (overhead mínimo) |

### 💼 Funciones como nodo maestro

```
┌─────────────────────────────────────────┐
│     NODO MAESTRO (Laptop B)             │
├─────────────────────────────────────────┤
│ ▶ Distribuir tareas a nodos secundarios │
│ ▶ Coordinar sincronización              │
│ ▶ Recopilar y validar resultados        │
│ ▶ Monitoreo de estado de red            │
│ ▶ Agregar estadísticas finales          │
└─────────────────────────────────────────┘
```

### ⚙️ Configuración recomendada

- **Hilos OpenMP:** 18 (configurar como predeterminado)
- **Máximo de imágenes:** 10 simultáneamente
- **Kernel blur:** 27 (balance calidad/velocidad)
- **Timeout entre sincronizaciones:** 5 segundos

---

## 2️⃣ Nodo Secundario de Alto Rendimiento: Laptop C (Yahel)

### 🏅 Razones técnicas

| Factor | Evaluación |
|---|---|
| **Rendimiento** | ⭐⭐⭐⭐⭐ Casi igual a Laptop B (6.9550s) |
| **Escalabilidad** | ⭐⭐⭐⭐ Buena, pero con anomalía a 12 hilos |
| **Estabilidad** | ⭐⭐⭐ Media (thermal throttling observable) |
| **Hardware** | ⭐⭐⭐⭐⭐ Mejor especificación (10 P+E cores) |
| **Sistema Operativo** | ⭐⭐⭐ Windows 11 (overhead medio) |

### ⚠️ Limitaciones observadas

- Thermal throttling a 12 hilos (+8% regresión)
- Mayor variabilidad entre ejecuciones
- Vulnerabilidad a procesos background

### 💼 Funciones como nodo secundario

```
┌─────────────────────────────────────────┐
│     NODO SECUNDARIO (Laptop C)          │
├─────────────────────────────────────────┤
│ ▶ Procesar cargas balanceadas           │
│ ▶ Generar transformaciones en paralelo  │
│ ▶ Reportar estado al maestro            │
│ ▶ Mantener cache local de resultados    │
└─────────────────────────────────────────┘
```

### ⚙️ Configuración recomendada

- **Hilos OpenMP:** 18 (crítico, evitar thermal throttling)
- **Máximo de imágenes:** 8 simultáneamente (límite conservador)
- **Monitoreo:** Temperatura CPU > 80°C → throttle a 12 hilos
- **Kernel blur:** 27
- **Sistema de refrigeración:** Asegurar ventilación adecuada

---

## 3️⃣ Nodo Secundario de Apoyo: Laptop A (Alejandro)

### 🏅 Razones técnicas

| Factor | Evaluación |
|---|---|
| **Rendimiento** | ⭐⭐ Menor velocidad (9.6230s) |
| **Escalabilidad** | ⭐⭐ Limitada por hardware (6 hilos físicos) |
| **Estabilidad** | ⭐⭐⭐⭐ Buena, sin anomalías |
| **Hardware** | ⭐⭐ Especificación básica (6 cores, sin HT) |
| **Sistema Operativo** | ⭐⭐⭐ Windows 11 |

### ⚠️ Limitaciones

- **No tiene hyperthreading** → Máximo 6 hilos reales
- Procesador generación anterior (Zen 3 vs Zen 4)
- 40% más lento que Laptop B

### 💼 Funciones como nodo de apoyo

```
┌─────────────────────────────────────────┐
│     NODO DE APOYO (Laptop A)            │
├─────────────────────────────────────────┤
│ ▶ Procesar lotes pequeños (1-2 imgs)   │
│ ▶ Tareas de bajo peso computacional     │
│ ▶ Procesamiento cuando otros no aplican │
│ ▶ Fallback/redundancia                  │
└─────────────────────────────────────────┘
```

### ⚙️ Configuración recomendada

- **Hilos OpenMP:** 12 (18 sería desperdicio)
- **Máximo de imágenes:** 2-3 simultáneamente
- **Kernel blur:** 21 (kernel menor para mayor velocidad)
- **Uso:** Procesar en paralelo mientras A y C trabajan en tareas grandes

---

## 📋 Recomendaciones de deployment

### Arquitectura de red

```
                    ┌─────────────────────┐
                    │   NODO MAESTRO      │
                    │  Laptop B (6.83s)   │
                    └──────────┬──────────┘
                               │
                ┌──────────────┼──────────────┐
                │              │              │
          ┌─────▼────────┐ ┌──▼───────────┐ ┌─▼──────────────┐
          │   SECUNDARIO │ │  SECUNDARIO  │ │  APOYO        │
          │    PREMIUM   │ │  (Fallback)  │ │  (Batch)      │
          │ Laptop C     │ │  (en reserve)│ │  Laptop A     │
          │  (6.95s)     │ │              │ │  (9.62s)      │
          └──────────────┘ └──────────────┘ └───────────────┘
```

### Distribución de carga

| Tipo de tarea | Asignación | Tiempo esperado |
|---|---|---|
| Grande (10 imgs × 6 filtros) | Maestro → Secundario Premium | ~7s |
| Mediana (6 imgs × 6 filtros) | Secundario Premium | ~4.5s |
| Pequeña (2 imgs × 6 filtros) | Nodo Apoyo | ~2s |
| Urgente (1 img × 1 filtro) | Maestro (local) | ~0.5s |

### Sincronización

```c
// En el coordinador:
if (Laptop_B.available) {
    assign_to_master(BIG_BATCH);           // 10 imágenes
} else if (Laptop_C.available) {
    assign_to_secondary_premium(MED_BATCH); // 6 imágenes
} else {
    assign_to_secondary_support(SMALL_BATCH); // 2 imágenes
}
```

### Parámetros globales

| Parámetro | Recomendación | Justificación |
|---|---|---|
| **Hilos globales** | 18 | Punto óptimo para todos |
| **Kernel blur** | 27 | Balance calidad/velocidad |
| **Max imágenes** | 10 (agregado) | Prevenir sobrecarga |
| **Timeout** | 30s | Permitir I/O sin timeout |
| **Reintento** | 3 veces | Tolerancia a fallos |

---

## ✅ Validaciones antes de producción

- [ ] Testear con imágenes de diferentes tamaños (320×240, 800×600, 1920×1080)
- [ ] Validar que `img/` se crea correctamente en cada nodo
- [ ] Medir latencia de red entre nodos (debe ser < 100ms)
- [ ] Crear fallback si Nodo B no está disponible
- [ ] Monitorear temperatura en Laptop C durante 24h
- [ ] Documentar procedimiento de escalado manual

---

[[4-Análisis-Detallado]] | [[Home]] | [[6-Arquitectura]]
