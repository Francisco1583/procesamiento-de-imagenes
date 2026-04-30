> [!IMPORTANT]  
> Este experimento busca comparar el desempeño de los múltiples dispositivos del equipo para identificar cuál sería el nodo principal y los nodos secundarios para llevar a cabo la transformación de imágenes.

## Abstract

> Este trabajo evalúa el rendimiento de una implementación paralela de múltiples transformaciones a 4 imágenes BMP, analizando el comportamiento del tiempo de ejecución en función del número de hilos en 3 entornos de hardware distintos. El estudio examina la eficiencia de los dispositivos bajo diferentes configuraciones de concurrencia (6, 12 y 18 hilos) utilizando OpenMP, identificando cuál es capaz de procesar las cargas en el menor tiempo.
>
> Los resultados muestran que el incremento de hilos hasta 18 redujo el tiempo de ejecución en los tres equipos evaluados. El análisis destaca a un equipo en particular por registrar la menor cantidad de tiempo total, haciéndolo el más adecuado para gestionar los procesos principales a través de la red.
>
> Asimismo, la comparación pone de manifiesto la influencia del sistema operativo, el número de núcleos físicos y la distribución de tareas de OpenMP en la eficiencia del programa paralelo.

## 1. Objetivo

Este documento analiza el rendimiento de las computadoras del equipo al ejecutar un script en C de transformaciones de imágenes en paralelo, comparando los tiempos obtenidos al variar el número de hilos. El propósito es identificar:

* ¿Cómo afecta el incremento de hilos (6, 12 y 18) en el tiempo de ejecución de cada integrante?
* De los dispositivos del equipo, ¿cuál es el mejor candidato para operar como nodo maestro en las transformaciones finales?
* ¿Qué hardware presenta la mejor estabilidad y escalabilidad?

---

## 2. Contexto experimental

### 2.1 Programa utilizado

Se utilizó un único código fuente (`para_image_parra.c`) que implementa la modificación de imágenes desde la memoria. El programa lee 4 imágenes BMP y les aplica 6 filtros distintos:
1. Inversión vertical en escala de grises.
2. Inversión horizontal (espejo) en escala de grises.
3. Inversión vertical a color.
4. Inversión horizontal (espejo) a color.
5. Desenfoque en escala de grises.
6. Desenfoque a color.

* Se utilizó **OpenMP** para paralelizar el trabajo. En lugar de un bucle for, el código emplea directivas `#pragma omp sections` y `#pragma omp section` para generar un total de **24 tareas independientes** (6 filtros x 4 imágenes).
* Se usó `omp_get_wtime()` para medir con precisión el tiempo de inicio y fin del procesamiento paralelo.

### 2.2 Hardware del equipo

Las pruebas se ejecutaron dentro de las siguientes máquinas, asignando un ID a cada una para su fácil lectura:

| ID | Persona | Sistema Operativo | RAM | Hilos lógicos | Núcleos físicos | Frecuencia | Procesador |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **A** | Alejandro | Windows 11 Home | 16 GB | 6 | 6 | Base | AMD Ryzen 5 4500U |
| **B** | Francisco | Linux Bodhi | 16 GB | 8 | 4 | 2.4 - 4.2 GHz | Intel Core i5-1135G7 (11 Gen) |
| **C** | Yahel | Windows 11 | 16 GB | 12 | 10 | 1.3 - 4.4 GHz | Intel Core i5-1235U (12 Gen) |

Diferencias estructurales relevantes para la interpretación de resultados:
* La laptop `C` (Yahel) cuenta con la mayor cantidad de núcleos físicos e hilos lógicos.
* La laptop `A` (Alejandro) no cuenta con hyperthreading (1 núcleo = 1 hilo).
* La laptop `B` (Francisco) ejecuta Linux (Bodhi), sistema operativo con un planificador de procesos más ligero y eficiente en comparación con Windows.

---

## 3. Datos medidos

Se realizaron 3 ejecuciones por integrante, variando la cantidad de hilos definidos en OpenMP a **6, 12 y 18 hilos**.

### 3.1 Tabla consolidada de tiempos (Segundos)

| Hilos | Laptop A (Alejandro) | Laptop B (Francisco) | Laptop C (Yahel) |
| ---: | :--- | :--- | :--- |
| **6** | 10.5160 | 7.3627 | 7.7310 |
| **12** | 9.8120 | 7.1117 | 8.2830 |
| **18** | 9.6230 | **6.8342** | 6.9550 |

> [!WARNING]  
> Los resultados dependen de los procesos en segundo plano de cada sistema operativo en el momento de la ejecución. Pequeñas variaciones, especialmente en Windows, pueden causar anomalías en los tiempos registrados.

---

## 4. Resumen ejecutivo

### Hallazgos principales

* **La laptop `B` (Francisco) obtuvo los mejores resultados en todas las configuraciones**, registrando el tiempo más bajo del experimento: **6.8342 segundos a 18 hilos**.
* **El rendimiento mejoró de manera continua hasta los 18 hilos** para todos los integrantes, con la excepción de un incremento de tiempo en la laptop C a 12 hilos.
* **La laptop `A` (Alejandro) registró los mayores tiempos de ejecución** en todas las pruebas, manteniéndose en el rango de los 9.6 a 10.5 segundos.
* **La laptop `C` (Yahel) presenta tiempos similares a la laptop B**, logrando 6.9550s en su mejor ejecución, aunque mostró inestabilidad en la prueba de 12 hilos.

---

## 5. Análisis por integrante

### 5.1 Alejandro (Laptop A)
* **Mejor resultado:** 9.6230 s (18 hilos)
* **Peor resultado:** 10.5160 s (6 hilos)

**Interpretación:** El equipo de Alejandro presenta una reducción de tiempos progresiva. A pesar de contar físicamente con solo 6 hilos lógicos (sin hyperthreading), OpenMP logra optimizar el proceso al asignar concurrencia hasta 18 hilos. No obstante, las especificaciones del procesador limitan el rendimiento máximo, posicionándolo con los tiempos más altos del equipo.

### 5.2 Francisco (Laptop B)
* **Mejor resultado:** 6.8342 s (18 hilos)
* **Peor resultado:** 7.3627 s (6 hilos)

**Interpretación:** La laptop de Francisco muestra un escalamiento eficiente y el mejor desempeño global. Aunque dispone de 4 núcleos físicos y 8 lógicos, el entorno de Linux Bodhi gestiona la memoria, las operaciones de entrada/salida (I/O) y los hilos de manera más efectiva que Windows. Esto permite maximizar la paralelización y reducir el tiempo sistemáticamente en cada incremento de hilos.

### 5.3 Yahel (Laptop C)
* **Mejor resultado:** 6.9550 s (18 hilos)
* **Peor resultado:** 8.2830 s (12 hilos)

**Interpretación:** El procesador de 12va generación de Yahel posee las especificaciones más altas (10 núcleos, 12 hilos), obteniendo un inicio rápido a 6 hilos (7.73s). Sin embargo, presenta un incremento de tiempo a los 12 hilos. Este comportamiento es frecuente en Windows 11 debido a tareas en segundo plano o limitación térmica (thermal throttling). Al configurar 18 hilos, el hardware alcanza su mejor desempeño (6.95s), aproximándose a los resultados del equipo B.

---

## 6. Interpretación técnica del comportamiento

### 6.1 ¿Por qué 18 hilos fue el mejor escenario para todos?
Bajo principios generales de paralelismo, superar el número de núcleos físicos puede incrementar el overhead y afectar negativamente el tiempo. Sin embargo, este código utiliza `#pragma omp sections` con 24 secciones independientes. 
* A 6 hilos, el sistema asigna a cada hilo 4 tareas de alta demanda en lectura/escritura de disco.
* A 18 hilos, la carga se distribuye de manera más granular (procesando en su mayoría 1 imagen/filtro por hilo). Al tratarse de tareas limitadas por operaciones de entrada/salida (I/O bound), el sistema operativo puede alternar hilos activos mientras otros esperan la escritura en el disco, haciendo de los 18 hilos el punto óptimo.

### 6.2 El factor Sistema Operativo
Los resultados indican que Linux Bodhi (Laptop B) presenta una ventaja considerable sobre Windows 11. Aunque la Laptop C posee un procesador con más núcleos físicos (10 vs 4), la Laptop B registró menores tiempos. Windows 11 introduce mayor latencia en las llamadas al sistema para operaciones binarias de I/O y cuenta con un planificador de CPU más demandante.

---

## 7. Conclusiones

1.  **Nodo Maestro:** La **Laptop `B` (Francisco)** es el candidato más adecuado para operar como el nodo maestro o distribuidor principal. Su combinación de procesador Intel de 11va generación con Linux Bodhi demostró la mayor rapidez, consistencia y eficiencia en la gestión del código y operaciones en memoria.
2.  **Nodo Secundario de Alto Rendimiento:** La **Laptop `C` (Yahel)** funcionará de manera óptima como un nodo secundario de primer nivel. Con la configuración a 18 hilos, procesa cargas a una velocidad similar a la del nodo maestro, aportando una alta capacidad de cómputo a la red.
3.  **Nodo Secundario de Apoyo:** La **Laptop `A` (Alejandro)** aporta un procesamiento estable. Debido a sus especificaciones físicas, es recomendable asignarle lotes de imágenes proporcionales a su capacidad para evitar retrasos en el tiempo total de la ejecución distribuida.
4.  **Configuración del Código:** Para el despliegue de la solución, la directiva de hilos debe configurarse en valores altos (18 o 24) en todos los equipos involucrados, ya que maximizar la concurrencia permite reducir los tiempos de espera generados por las operaciones de disco.

---

## 8. GUI build and execution

The graphical interface is implemented in `interfaz_grafica.c` and is designed to be built per platform with the same source file:

### Windows

```bash
gcc "interfaz_grafica.c" -o "interfaz_grafica.exe" -fopenmp -lgdi32 -lcomdlg32 -lshell32 -mwindows
```

### Linux and macOS

```bash
gcc "interfaz_grafica.c" -o "interfaz_grafica" `pkg-config --cflags --libs gtk+-3.0` -fopenmp
```

Notes:

* The application saves all generated BMP files inside the `img` folder located next to the executable.
* The interface shows the resolved output directory on screen.
* The interface also displays the total execution time after the selected transformations finish.