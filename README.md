> [!IMPORTANT]  
> Este experimento inicial evaluó el desempeño de los múltiples dispositivos del equipo para identificar cuál sería el nodo principal. Basado en estos hallazgos, el proyecto evolucionó hacia un **Clúster Distribuido Virtualizado** utilizando MPI, OpenMP y una interfaz en Python para procesar cargas masivas de hasta 150 imágenes.

## Abstract

> Este trabajo evalúa inicialmente el rendimiento de una implementación paralela de transformaciones de imágenes, analizando el comportamiento del tiempo de ejecución en función del número de hilos en 3 entornos de hardware distintos. Tras identificar el hardware más capaz, el proyecto se escaló a una **arquitectura híbrida distribuida (MPI + OpenMP)** montada sobre 3 Máquinas Virtuales interconectadas mediante NFS.
>
> Los resultados iniciales mostraron que el incremento de hilos hasta 18 redujo el tiempo de ejecución en los equipos evaluados, destacando un equipo en particular para gestionar el clúster. La implementación final orquesta el procesamiento a través de una Interfaz Gráfica (PyQt5) que incluye tolerancia a fallos, balanceo de carga asimétrico y sincronización en tiempo real.

## 1. Objetivo

Este documento analiza en una primera fase el rendimiento de las computadoras del equipo al ejecutar transformaciones en paralelo, para luego documentar el despliegue de la arquitectura de supercómputo final. El propósito abarca:

* ¿Cómo afecta el incremento de hilos en el tiempo de ejecución de cada integrante?
* Identificar el hardware con la mejor estabilidad para operar como el anfitrión (Host) del clúster de máquinas virtuales.
* Desplegar un sistema distribuido capaz de procesar hasta 150 imágenes con 6 transformaciones simultáneas (900 archivos) sin colapsos de memoria.

---

## 2. Contexto experimental

### 2.1 Programa utilizado y Evolución

En su versión final, el código fuente (`para_image_parra.c`) fue adaptado para un entorno de paso de mensajes (MPI) y memoria compartida (OpenMP). El programa lee imágenes BMP y les aplica 6 filtros distintos:
1. Inversión vertical en escala de grises.
2. Inversión vertical a color.
3. Inversión horizontal (espejo) en escala de grises.
4. Inversión horizontal (espejo) a color.
5. Desenfoque en escala de grises.
6. Desenfoque a color.

* **MPI (Message Passing Interface):** Distribuye el lote de imágenes (hasta 150) entre los distintos nodos (Ranks) del clúster.
* **OpenMP:** Paraleliza las 6 transformaciones a nivel de núcleo para cada imagen asignada al proceso.

### 2.2 Hardware del equipo (Fase de Evaluación)

Las pruebas preliminares se ejecutaron en las siguientes máquinas, asignando un ID a cada una para su fácil lectura:

| ID | Persona | Sistema Operativo | RAM | Hilos lógicos | Núcleos físicos | Frecuencia | Procesador |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **A** | Alejandro | Windows 11 Home | 16 GB | 6 | 6 | Base | AMD Ryzen 5 4500U |
| **B** | Francisco | Linux Bodhi | 16 GB | 8 | 4 | 2.4 - 4.2 GHz | Intel Core i5-1135G7 (11 Gen) |
| **C** | Yahel | Windows 11 | 16 GB | 12 | 10 | 1.3 - 4.4 GHz | Intel Core i5-1235U (12 Gen) | 

*(Nota: Para la arquitectura final, el host principal se actualizó a un ASUS ROG Strix con procesador i7-13650hx de 14 núcleos físicos y 20 hilos lógicos).*

---

## 3. Datos medidos (Benchmark Preliminar)

Se realizaron 3 ejecuciones por integrante, variando la cantidad de hilos definidos en OpenMP a **6, 12 y 18 hilos**.

### 3.1 Tabla consolidada de tiempos (Segundos)

| Hilos | Laptop A (Alejandro) | Laptop B (Francisco) | Laptop C (Yahel) |
| ---: | :--- | :--- | :--- |
| **6** | 10.5160 | 7.3627 | 7.7310 |
| **12** | 9.8120 | 7.1117 | 8.2830 |
| **18** | 9.6230 | **6.8342** | 6.9550 |

> [!WARNING]  
> Los resultados dependen de los procesos en segundo plano de cada sistema. Estas pruebas justificaron centralizar el procesamiento final en el hardware más robusto mediante virtualización.

---

## 4. Resumen ejecutivo

### Hallazgos principales

* **La laptop `B` (Francisco) obtuvo los mejores resultados en todas las configuraciones**, registrando el tiempo más bajo del experimento inicial: **6.8342 segundos a 18 hilos**.
* **El rendimiento mejoró de manera continua hasta los 18 hilos** para todos los integrantes, demostrando la escalabilidad del código.
* **La laptop `A` (Alejandro) registró los mayores tiempos de ejecución**, manteniéndose en el rango de los 9.6 a 10.5 segundos.
* **El hardware de Yahel (Laptop C)**, al poseer la mayor cantidad de núcleos e hilos lógicos y gran rendimiento térmico, fue designado finalmente como el Host físico ideal para albergar el clúster de 3 Máquinas Virtuales (Master, Esclavo1, Esclavo2).

---

## 5. Análisis por integrante

### 5.1 Alejandro (Laptop A)
* **Mejor resultado:** 9.6230 s (18 hilos)
* **Peor resultado:** 10.5160 s (6 hilos)

**Interpretación:** El equipo de Alejandro presenta una reducción de tiempos progresiva. A pesar de contar físicamente con solo 6 hilos lógicos (sin hyperthreading), OpenMP logra optimizar el proceso al asignar concurrencia hasta 18 hilos. No obstante, las especificaciones del procesador limitan el rendimiento máximo.

### 5.2 Francisco (Laptop B)
* **Mejor resultado:** 6.8342 s (18 hilos)
* **Peor resultado:** 7.3627 s (6 hilos)

**Interpretación:** La laptop de Francisco muestra un escalamiento eficiente. Aunque dispone de 4 núcleos físicos y 8 lógicos, el entorno de Linux Bodhi gestiona la memoria y las operaciones de entrada/salida (I/O) de manera más efectiva que Windows en esta prueba base.

### 5.3 Yahel (Laptop C)
* **Mejor resultado:** 6.9550 s (18 hilos)
* **Peor resultado:** 8.2830 s (12 hilos)

**Interpretación:** El procesador de Yahel posee las especificaciones más altas, obteniendo un inicio rápido a 6 hilos. Al configurar 18 hilos, el hardware alcanza su mejor desempeño (6.95s), lo que lo convierte en el anfitrión ideal para virtualizar el clúster completo.

---

## 6. Interpretación técnica del comportamiento

### 6.1 ¿Por qué 18 hilos fue el mejor escenario inicial?
Bajo principios generales de paralelismo, superar el número de núcleos físicos puede incrementar el overhead. Sin embargo, al tratarse de tareas limitadas por operaciones de entrada/salida (I/O bound), el sistema operativo puede alternar hilos activos mientras otros esperan la escritura en el disco, haciendo de los 18 hilos el punto óptimo en la versión de código pura de OpenMP.

### 6.2 El factor Sistema Operativo
Los resultados indicaron que Linux maneja con menor latencia las llamadas al sistema para operaciones binarias de I/O. Esto justificó la decisión de montar las 3 máquinas virtuales de la arquitectura final utilizando Kali Linux.

---

## 7. Conclusiones Arquitectónicas

La prueba de hardware nos llevó a la topología final del sistema distribuido:
1.  **Concentración de Poder:** En lugar de usar una red física propensa a latencias, se optó por crear 3 VMs dentro del equipo de mayor rendimiento local, logrando transferencias de archivos pesados a velocidades del conmutador virtual de memoria.
2.  **Afinidad de Hilos Controlada:** Para el código distribuido, se limitó OpenMP a configuraciones simétricas por proceso (ej. 1 o 2 hilos) para evitar la sobresuscripción ("Thread Thrashing") contra los hilos lógicos reales del procesador anfitrión al correr MPI.

---

## 8. Arquitectura y flujo de datos: Sistema Híbrido y UI

El proyecto ha evolucionado hacia un orquestador en Python que controla el backend distribuido en C.

### Estructura de Capas

```text
Interfaz.py (PyQt5 UI - Orquestador)
    ↓ (Captura imágenes y parámetros)
subprocess.Popen("mpirun ...")
    ↓ (MPI distribuye imágenes entre VMs)
main_mpi (Backend C: para_image_parra.c)
    ↓ (OpenMP paraleliza 6 transformaciones por imagen)
NFS (/home/kali/proyecto)
    ↓ (Mini-Bulk Transfer asíncrono)
UI lee stdout (Actualiza Barra de Progreso)
```

### Flujo de Ejecución y Tolerancia a Fallos:

1. **Interacción del Usuario:** Se seleccionan hasta 150 archivos BMP y se marcan las transformaciones deseadas en la UI.
2. **Orquestación MPI:** La interfaz ejecuta el backend en C utilizando `mpirun` referenciando el archivo `hosts_mpi` para balancear asimétricamente la carga.
3. **Procesamiento y Red:** Cada nodo procesa su cuota de imágenes usando OpenMP y envía los resultados en bloques pequeños al NFS mediante un *Mini-Bulk Transfer* iterativo (`mv -v /tmp/*_%d.bmp ...`) para evitar el desbordamiento de la memoria RAM (Swap death).
4. **Fallback Automático:** Si un nodo virtual se desconecta, `mpirun` abortará. El script de Python detecta la caída, filtra el directorio de resultados para ver qué se salvó, y relanza un entorno de rescate aislado (`-host localhost`) para procesar únicamente las imágenes faltantes de forma transparente.

---

## 9. Instrucciones de Configuración y Despliegue (Clúster NFS + MPI)

Para montar correctamente el ecosistema de red y los servicios de distribución en las 3 máquinas virtuales (Kali Linux), siga el siguiente orden de ejecución:

### 1. Configuración del Archivo Hosts (En TODOS los equipos)
Permite la resolución por nombre en lugar de por IP.
```bash
sudo nano /etc/hosts
# Añadir: IP_DEL_MASTER master
# Añadir: IP_DEL_ESCLAVO1 esclavo1 ...
```

### 2. Reinicio de Servicios Base (En el MASTER)
Garantizar la correcta operación de RPC y NFS antes de enlazar los equipos.
```bash
sudo systemctl restart rpcbind nfs-kernel-server
# Reset de servicios adicionales por si acaso
sudo systemctl restart nfs-kernel-server
```

### 3. Preparación del Sistema y Dependencias (En AMBOS ESCLAVOS)
```bash
# Definir variable de entorno
DestinationDirectory="/home/kali/proyecto"

# Crear directorio y asegurar permisos totales para NFS
mkdir -p $DestinationDirectory
sudo chmod 777 $DestinationDirectory

# Actualizar en caso de que sea necesario e instalar MPI y NFS
sudo apt update
sudo apt install -y mpich libmpich-dev nfs-kernel-server nfs-common
```

### 4. Montaje de la Red de Almacenamiento (En AMBOS ESCLAVOS)
El montaje se debe hacer de forma asíncrona (`async`) para evitar cuellos de botella de entrada/salida (I/O) en Linux.
```bash
# Opcional: Desmontar montaje previo si fue síncrono
sudo umount /home/kali/proyecto

# Montaje oficial con async
sudo mount -t nfs -o rw,async master:$DestinationDirectory $DestinationDirectory

# Verificación
ls -l $DestinationDirectory
```

### 5. Compilación del Backend MPI (En el MASTER)
Compilar el código principal en C habilitando la bandera de OpenMP.
```bash
mpicc -fopenmp para_image_parra.c -o main_mpi
```

### 6. Ejecución del Entorno Gráfico (En el MASTER)
Con todas las dependencias en red corriendo, lanzar la interfaz gráfica:
```bash
python3 Interfaz.py
```