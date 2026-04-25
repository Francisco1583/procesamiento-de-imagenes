> [!IMPORTANT]  
> Este experimento busca comparar el desempeño de los múltiples dispositivos del equipo para identificar cuál sería el "amo" y los "esclavos" para llevar a cabo la transformación de imágenes.

## Abstract

> Este trabajo evalúa el rendimiento de una implementación paralela de múltiples transformaciones a 4 imágenes BMP, analizando el comportamiento del tiempo de ejecución en función del número de hilos en 3 entornos de hardware distintos. El estudio examina la eficiencia de los dispositivos bajo diferentes configuraciones de concurrencia (6, 12 y 18 hilos) utilizando OpenMP, identificando cuál es capaz de soportar las cargas en un menor tiempo.
>
> Los resultados muestran que, a diferencia de otros experimentos, el incremento de hilos hasta 18 redujo el tiempo de ejecución en los tres equipos evaluados, destacando a un equipo en particular por requerir la menor cantidad de tiempo total. Esto lo hace el más adecuado para gestionar los procesos principales a través de la red.
>
> Asimismo, el análisis comparativo pone de manifiesto la influencia del sistema operativo, el número de núcleos físicos y la distribución de tareas de OpenMP en la eficiencia del programa paralelo.

## 1. Objetivo

Este documento analiza el rendimiento de las computadoras del equipo al ejecutar un script en C de transformaciones de imágenes en paralelo, comparando los tiempos obtenidos al variar el número de hilos. El propósito es identificar:

* ¿Cómo afecta el incremento de hilos (6, 12 y 18) en el tiempo de ejecución de cada integrante?
* De los dispositivos del equipo, ¿cuál es el mejor candidato para fungir como nodo maestro en las transformaciones finales?
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

Esto ya sugiere una base importante para interpretar los resultados:
* La laptop `C` (Yahel) cuenta con la mayor cantidad de núcleos físicos e hilos lógicos.
* La laptop `A` (Alejandro) no cuenta con hyperthreading (1 núcleo = 1 hilo).
* La laptop `B` (Francisco) corre bajo un entorno Linux (Bodhi), conocido por ser ligero y tener un planificador de procesos (scheduler) muy eficiente, a diferencia de Windows.

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
> Los resultados dependen de los procesos en segundo plano de cada sistema operativo en el momento de la ejecución. Pequeñas variaciones, especialmente en Windows, pueden causar picos atípicos.

---

## 4. Resumen ejecutivo

### Hallazgos principales

* **La laptop `B` (Francisco) obtuvo los mejores tiempos en todas las configuraciones**, coronándose con el tiempo más bajo de todo el experimento: **6.8342 segundos a 18 hilos**.
* **A diferencia de otros reportes, el rendimiento continuó mejorando hasta los 18 hilos** para todos los integrantes (con la excepción de una anomalía en la laptop C a 12 hilos).
* **La laptop `A` (Alejandro) requirió el mayor tiempo de ejecución** en todas las rondas, manteniéndose en el rango de los 9.6 a 10.5 segundos.
* **La laptop `C` (Yahel) compite de cerca con la laptop B**, logrando bajar de los 7 segundos en su mejor ejecución (6.9550s), pero mostró inestabilidad en la prueba de 12 hilos.

---

## 5. Análisis por integrante

### 5.1 Alejandro (Laptop A)
* **Mejor resultado:** 9.6230 s (18 hilos)
* **Peor resultado:** 10.5160 s (6 hilos)

**Interpretación:** La máquina de Alejandro muestra una mejora escalonada y muy estable. A pesar de contar físicamente con solo 6 hilos lógicos (sin hyperthreading), OpenMP logra reducir los tiempos al asignarle más hilos virtuales (hasta 18). Sin embargo, el procesador hace "cuello de botella", manteniéndolo como el dispositivo con los tiempos más altos del equipo.

### 5.2 Francisco (Laptop B)
* **Mejor resultado:** 6.8342 s (18 hilos)
* **Peor resultado:** 7.3627 s (6 hilos)

**Interpretación:** La laptop de Francisco presenta una curva de optimización perfecta y el rendimiento más veloz. Aunque solo tiene 4 núcleos físicos y 8 lógicos, el entorno de **Linux Bodhi** gestiona la memoria, el I/O del disco y los hilos de manera muy superior a Windows. Esto le permite aprovechar la paralelización al máximo, rebajando el tiempo con cada incremento de hilos.

### 5.3 Yahel (Laptop C)
* **Mejor resultado:** 6.9550 s (18 hilos)
* **Peor resultado:** 8.2830 s (12 hilos)

**Interpretación:** El i5 de 12va generación de Yahel tiene el hardware más potente (10 núcleos, 12 hilos), por lo que arranca muy rápido a 6 hilos (7.73s). Sin embargo, sufre un salto atípico de lentitud a los 12 hilos. Esto es muy común en Windows 11 debido a tareas en segundo plano o *thermal throttling* (aceleración térmica). Al pasar a 18 hilos, el hardware desata su potencial y logra un tiempo excelente de 6.95s, pegado al mejor tiempo de Francisco.

---

## 6. Interpretación técnica del comportamiento

### 6.1 ¿Por qué 18 hilos fue el mejor escenario para todos?
En la teoría general de paralelismo, superar el número de núcleos físicos suele incrementar el overhead y empeorar el tiempo. Sin embargo, en nuestro código específico usamos `#pragma omp sections` con 24 secciones independientes. 
* A 6 hilos, a cada hilo le tocan 4 pesadas tareas de lectura/escritura de imágenes.
* A 18 hilos, la carga se distribuye mucho más finamente (la mayoría de los hilos procesan solo 1 imagen/filtro). Al ser tareas con alta carga de I/O (I/O bound), mientras un hilo espera a que el disco duro escriba el `.bmp`, el sistema operativo le da paso a otro hilo. Esto hizo que 18 hilos fuera el punto más óptimo.

### 6.2 El factor Sistema Operativo
Queda claro que Linux Bodhi (Laptop B) otorgó una ventaja abismal sobre Windows 11. Aunque la Laptop C de Yahel tenía un hardware superior (10 núcleos vs 4 núcleos), Francisco ganó todas las pruebas. Windows 11 introduce mayor latencia en las llamadas al sistema para leer/escribir archivos binarios y tiene un planificador de CPU más pesado.

---

## 7. Conclusiones

1.  **Nodo Maestro Ideal:** La **Laptop `B` (Francisco)** es sin duda el mejor candidato para fungir como el nodo maestro o principal distribuidor. Su combinación de procesador Intel de 11va generación con Linux Bodhi demostró ser la más rápida, consistente y eficiente para lidiar con el código y las operaciones de memoria.
2.  **Nodo Esclavo de Alto Rendimiento:** La **Laptop `C` (Yahel)** actuará como un esclavo de primer nivel. Con 18 hilos es capaz de procesar cargas a casi la misma velocidad que el maestro, brindando un tremendo poder de cómputo en la red.
3.  **Nodo Esclavo de Apoyo:** La **Laptop `A` (Alejandro)**, si bien es la más lenta debido a su limitación de núcleos/hilos físicos, aporta procesamiento estable. Se le pueden asignar lotes un poco más pequeños de imágenes a procesar para que no retrase el tiempo total de la ejecución distribuida.
4.  **Configuración del Código:** Para el despliegue final, la constante `NUM_THREADS` debe configurarse en valores altos (como 18 o 24) en todos los equipos, ya que se comprobó que saturar los hilos reduce los tiempos muertos de I/O de disco.