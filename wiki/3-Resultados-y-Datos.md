# Resultados y Datos

## Tabla consolidada de tiempos (segundos)

| Hilos | Laptop A (Alejandro) | Laptop B (Francisco) | Laptop C (Yahel) |
|:---:|:---:|:---:|:---:|
| **6** | 10.5160 | 7.3627 | 7.7310 |
| **12** | 9.8120 | 7.1117 | 8.2830 |
| **18** | 9.6230 | **6.8342** ⭐ | 6.9550 |

### 📊 Visualización de tendencias

```
LAPTOP A (AMD Ryzen 5 4500U - Windows)
10.5160 ──────────────
  9.8120 ─────────
  9.6230 ────────

LAPTOP B (Intel i5-1135G7 - Linux) ⭐ MEJOR
 7.3627 ────────────
 7.1117 ──────────
 6.8342 ────────

LAPTOP C (Intel i5-1235U - Windows)
 7.7310 ────────────
 8.2830 ──────────────  ⚠️ Anomalía a 12 hilos
 6.9550 ───────
```

---

## 🔍 Hallazgos principales

### ✅ La Laptop B obtuvo los mejores resultados

- **Mejor tiempo global:** 6.8342 segundos (18 hilos)
- **Mejor consistencia:** Reduce tiempo en cada incremento de hilos
- **Ventaja:** Linux Bodhi + Intel Gen 11 = máxima eficiencia

### ✅ Rendimiento mejoró hasta 18 hilos

| Dispositivo | Mejora 6→18 hilos |
|---|---|
| Laptop A | 8.4% |
| Laptop B | 7.1% |
| Laptop C | 9.9% |

**Conclusión:** El incremento de hilos es beneficioso en todos los casos, sin punto de saturación en 18.

### ⚠️ Laptop C registró anomalía a 12 hilos

- **6 hilos:** 7.7310s (rápido, buen inicio)
- **12 hilos:** 8.2830s (+8% más lento) ← **Regresión**
- **18 hilos:** 6.9550s (recuperación)

**Causa probable:** Thermal throttling o tareas en segundo plano en Windows 11

### ❌ Laptop A registró los mayores tiempos

- Rango: 9.6–10.5 segundos
- **Razón:** Solo 6 hilos físicos (sin hyperthreading)
- **Implicación:** Menos paralelismo disponible = cuello de botella

---

## 📈 Análisis de escalabilidad

### Cambio de tiempo al incrementar hilos (segundos de mejora)

| Paso | Laptop A | Laptop B | Laptop C |
|---|---|---|---|
| 6→12 hilos | -0.6040s (↓5.8%) | -0.2510s (↓3.4%) | +0.5520s (↑7.1%) ⚠️ |
| 12→18 hilos | -0.1890s (↓1.9%) | -0.2775s (↓3.9%) | -1.3280s (↓16.0%) |
| **Total 6→18** | -0.7930s (↓7.5%) | -0.5285s (↓7.2%) | -0.7760s (↓10.0%) |

**Factor importante:** A pesar de que Laptop C es más potente, Windows introduce inestabilidad que se compensa solo al llegar a 18 hilos.

---

## 📋 Consideraciones sobre medición

> ⚠️ Los resultados dependen de procesos en segundo plano del sistema operativo en el momento de ejecución.  
> Pequeñas variaciones, especialmente en Windows, pueden causar anomalías de ±0.5 segundos.

**Mitigation employed:**
- 3 repeticiones por configuración (reducir outliers)
- Ejecución en horarios similares
- Sin navegadores ni aplicaciones pesadas abiertas

---

[[2-Contexto-Experimental]] | [[Home]] | [[4-Análisis-Detallado]]
