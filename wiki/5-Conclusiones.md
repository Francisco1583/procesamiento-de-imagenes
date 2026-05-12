# Conclusiones y Recomendaciones

## Resumen ejecutivo

La UI ahora solo coordina la ejecución, así que la decisión de nodo maestro depende casi por completo del backend y del hardware disponible. Con los datos medidos, el orden final no cambia:

- Nodo maestro: Laptop B (Francisco).
- Nodo secundario de alto rendimiento: Laptop C (Yahel).
- Nodo secundario de apoyo: Laptop A (Alejandro).

## Nodo maestro: Laptop B

La Laptop B sigue siendo la mejor opción por estabilidad y tiempos. Linux Bodhi reduce ruido del sistema operativo y el backend optimizado aprovecha mejor esa ventaja.

### Configuración recomendada
- Hilos OpenMP: 18.
- Máximo de imágenes: hasta 10 en la interfaz, aunque conviene usar lotes moderados.
- Kernel blur: 27 como valor base.

## Nodo secundario de alto rendimiento: Laptop C

La Laptop C ofrece tiempos muy cercanos al nodo maestro, pero con más variabilidad. Es útil como apoyo fuerte cuando se quiere distribuir carga grande sin sacrificar demasiado tiempo total.

### Configuración recomendada
- Hilos OpenMP: 18.
- Uso recomendado: cargas medianas y grandes.
- Precaución: evitar ejecutarla con mucha carga de fondo en Windows.

## Nodo secundario de apoyo: Laptop A

La Laptop A queda como la alternativa más estable para lotes pequeños o tareas auxiliares. El backend optimizado le ayuda, pero no elimina la limitación física de sus 6 hilos lógicos.

### Configuración recomendada
- Hilos OpenMP: 12 o 18 según disponibilidad.
- Uso recomendado: lotes pequeños o respaldo.

## Recomendación final

| Componente | Recomendación |
|---|---|
| Maestro | Laptop B |
| Secundario principal | Laptop C |
| Secundario de apoyo | Laptop A |
| Hilos recomendados | 18 |
| Kernel base | 27 |

---

[[4-Análisis-Detallado]] | [[Home]] | [[6-Arquitectura]]
