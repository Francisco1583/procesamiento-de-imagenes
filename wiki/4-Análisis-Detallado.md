# Análisis Detallado por Dispositivo

## Laptop A: Alejandro (AMD Ryzen 5 4500U)

| Configuración | Tiempo | Nota |
|---|---|---|
| 6 hilos | 10.5160s | Sin hyperthreading: techo práctico del equipo |
| 12 hilos | 9.8120s | Más paralelismo, pero con sobrecarga de planificación |
| 18 hilos | 9.6230s | Mejor resultado, aunque con margen limitado |

### Interpretación

La Laptop A sigue siendo la menos favorecida por hardware. El backend optimizado reduce el costo del I/O, pero no cambia el hecho de que el procesador tiene menos margen para esconder latencia cuando el número de tareas sube.

### Conclusión para Laptop A
- Rendimiento: bajo.
- Escalabilidad: limitada por hardware.
- Recomendación: nodo secundario de apoyo.
- Asignación ideal: lotes pequeños o tareas de menor prioridad.

---

## Laptop B: Francisco (Intel Core i5-1135G7 + Linux Bodhi)

| Configuración | Tiempo | Nota |
|---|---|---|
| 6 hilos | 7.3627s | Mejor arranque entre las tres máquinas |
| 12 hilos | 7.1117s | Mejora sostenida |
| 18 hilos | 6.8342s | Mejor resultado global |

### Interpretación

La Laptop B combina un sistema operativo con menor overhead y un backend que ya no repite lecturas ni escrituras innecesarias. Esa combinación explica por qué mantiene una tendencia estable y sin regresiones visibles.

### Conclusión para Laptop B
- Rendimiento: excelente.
- Escalabilidad: consistente y predecible.
- Estabilidad: alta.
- Recomendación: nodo maestro.

---

## Laptop C: Yahel (Intel Core i5-1235U)

| Configuración | Tiempo | Nota |
|---|---|---|
| 6 hilos | 7.7310s | Buen inicio |
| 12 hilos | 8.2830s | Regresión puntual |
| 18 hilos | 6.9550s | Recuperación fuerte |

### Interpretación

La Laptop C sigue mostrando el mejor hardware bruto, pero Windows introduce más variabilidad. El backend optimizado ayuda a que la recuperación a 18 hilos sea clara, aunque no elimina la irregularidad intermedia.

### Conclusión para Laptop C
- Rendimiento: alto.
- Escalabilidad: buena, pero menos estable que B.
- Estabilidad: media.
- Recomendación: nodo secundario de alto rendimiento.

---

## Factor Sistema Operativo

La diferencia entre Linux y Windows sigue siendo visible, pero ahora se ve más como una diferencia de ejecución alrededor del backend, no de la interfaz.

| Aspecto | Linux | Windows |
|---|---|---|
| Overhead del sistema | Bajo | Más alto |
| Servicios en segundo plano | Menos intrusivos | Más frecuentes |
| Context switching | Más estable | Más variable |
| Impacto en la prueba | Menor ruido | Más dispersiones |

## Punto óptimo de hilos

El mejor punto sigue siendo 18 hilos porque el backend trabaja con tareas independientes y reduce el costo de I/O. En este escenario, subir más hilos no garantiza una mejora adicional visible.

---

[[3-Resultados-y-Datos]] | [[Home]] | [[5-Conclusiones]]
