import sys
import os
import subprocess
from PyQt5.QtWidgets import (QApplication, QWidget, QVBoxLayout, QHBoxLayout, 
                             QLabel, QPushButton, QCheckBox, QLineEdit, 
                             QFileDialog, QMessageBox, QGridLayout, QTextEdit, QProgressBar)
from PyQt5.QtCore import Qt
from PyQt5.QtGui import QPixmap, QIntValidator

# =========================================================
# CLASE PERSONALIZADA PARA EL ÁREA DE ARRASTRAR Y SOLTAR
# =========================================================
class DropZone(QLabel):
    def __init__(self):
        super().__init__()
        self.archivos = []
        self.setText("Arrastra imágenes\nmáximo 10\n.bmp\n\n(O haz clic para seleccionar)")
        self.setAlignment(Qt.AlignCenter)
        self.setStyleSheet("""
            QLabel {
                border: 2px dashed #666;
                border-radius: 8px;
                background-color: #2b2b2b;
                color: #dddddd;
                font-size: 14px;
            }
            QLabel:hover { border: 2px dashed #999; background-color: #333333; }
        """)
        self.setAcceptDrops(True)
        self.setMinimumSize(250, 150)

    def dragEnterEvent(self, event):
        if event.mimeData().hasUrls():
            event.accept()
        else:
            event.ignore()

    def dropEvent(self, event):
        for url in event.mimeData().urls():
            path = url.toLocalFile()
            if path.lower().endswith('.bmp'):
                if len(self.archivos) < 10 and path not in self.archivos:
                    self.archivos.append(path)
                elif len(self.archivos) >= 10:
                    QMessageBox.warning(self, "Límite", "Solo puedes procesar hasta 10 imágenes a la vez.")
                    break
        self.actualizar_texto()

    def mousePressEvent(self, event):
        archivos_seleccionados, _ = QFileDialog.getOpenFileNames(
            self, "Seleccionar Imágenes BMP", "", "Imágenes BMP (*.bmp)")
        
        for path in archivos_seleccionados:
            if len(self.archivos) < 10 and path not in self.archivos:
                self.archivos.append(path)
        self.actualizar_texto()

    def actualizar_texto(self):
        if not self.archivos:
            self.setText("Arrastra imágenes\nmáximo 10\n.bmp\n\n(O haz clic para seleccionar)")
        else:
            nombres = [os.path.basename(p) for p in self.archivos]
            texto = f"{len(self.archivos)} imágenes listas:\n" + "\n".join(nombres[:5])
            if len(nombres) > 5:
                texto += "\n..."
            self.setText(texto)

# =========================================================
# VENTANA PRINCIPAL DE LA APLICACIÓN
# =========================================================
class AppProcesamiento(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Procesamiento de Imágenes")
        self.resize(700, 500)
        self.setStyleSheet("background-color: #1e1e1e; color: #eeeeee; font-family: Arial;")
        
        self.directorio_base = os.path.dirname(os.path.abspath(__file__))
        self.ruta_salida = os.path.join(self.directorio_base, "resultados")
        if not os.path.exists(self.ruta_salida):
            os.makedirs(self.ruta_salida)

        self.initUI()

    def initUI(self):
        layout_principal = QHBoxLayout()

        # ----- COLUMNA IZQUIERDA -----
        col_izq = QVBoxLayout()
        
        self.drop_zone = DropZone()
        col_izq.addWidget(self.drop_zone)
        
        col_izq.addSpacing(15)
        
        # Barra de progreso
        self.lbl_progreso = QLabel("Progreso del Clúster:")
        self.barra_progreso = QProgressBar()
        self.barra_progreso.setRange(0, 100)
        self.barra_progreso.setValue(0)
        self.barra_progreso.setStyleSheet("""
            QProgressBar { border: 1px solid #555; border-radius: 5px; text-align: center; color: white; font-weight: bold; }
            QProgressBar::chunk { background-color: #4CAF50; border-radius: 4px; }
        """)
        col_izq.addWidget(self.lbl_progreso)
        col_izq.addWidget(self.barra_progreso)
        
        col_izq.addSpacing(15)
        
        self.lbl_tiempo = QLabel("Tiempo de ejecución:")
        self.txt_tiempo = QLineEdit()
        self.txt_tiempo.setReadOnly(True)
        self.txt_tiempo.setStyleSheet("background-color: #333; border: 1px solid #555; padding: 5px;")
        col_izq.addWidget(self.lbl_tiempo)
        col_izq.addWidget(self.txt_tiempo)
        
        col_izq.addSpacing(10)
        
        self.lbl_ruta = QLabel("Ruta de archivos (Resultados):")
        self.txt_ruta = QLineEdit(self.ruta_salida)
        self.txt_ruta.setReadOnly(True)
        self.txt_ruta.setStyleSheet("background-color: #333; border: 1px solid #555; padding: 5px; font-size: 11px;")
        col_izq.addWidget(self.lbl_ruta)
        col_izq.addWidget(self.txt_ruta)
        
        col_izq.addSpacing(10)
        self.lbl_consola = QLabel("Monitor del Clúster (Salida MPI):")
        self.consola = QTextEdit()
        self.consola.setReadOnly(True)
        self.consola.setStyleSheet("background-color: #0c0c0c; color: #00ff00; font-family: monospace; border: 1px solid #555;")
        col_izq.addWidget(self.lbl_consola)
        col_izq.addWidget(self.consola)

        # ----- COLUMNA DERECHA -----
        col_der = QVBoxLayout()
        
        estilo_check = "QCheckBox { font-size: 14px; spacing: 10px; margin-bottom: 5px; }"
        
        self.chk1 = QCheckBox("1- Vertical escala de grises")
        self.chk2 = QCheckBox("2- Vertical escala a colores")
        self.chk3 = QCheckBox("3- Horizontal escala de grises")
        self.chk4 = QCheckBox("4- Horizontal escala a colores")
        
        for chk in [self.chk1, self.chk2, self.chk3, self.chk4]:
            chk.setStyleSheet(estilo_check)
            col_der.addWidget(chk)

        lay_blur_gris = QHBoxLayout()
        self.chk5 = QCheckBox("5- Desenfoque escala de grises")
        self.chk5.setStyleSheet(estilo_check)
        self.txt_k_gris = QLineEdit("27")
        self.txt_k_gris.setValidator(QIntValidator(1, 999))
        self.txt_k_gris.setFixedWidth(50)
        self.txt_k_gris.setStyleSheet("background-color: #333; border: 1px solid #555;")
        lay_blur_gris.addWidget(self.chk5)
        lay_blur_gris.addWidget(self.txt_k_gris)
        lay_blur_gris.addWidget(QLabel("Kernel"))
        lay_blur_gris.addStretch()
        col_der.addLayout(lay_blur_gris)

        lay_blur_color = QHBoxLayout()
        self.chk6 = QCheckBox("6- Desenfoque escala a colores")
        self.chk6.setStyleSheet(estilo_check)
        self.txt_k_color = QLineEdit("27")
        self.txt_k_color.setValidator(QIntValidator(1, 999))
        self.txt_k_color.setFixedWidth(50)
        self.txt_k_color.setStyleSheet("background-color: #333; border: 1px solid #555;")
        lay_blur_color.addWidget(self.chk6)
        lay_blur_color.addWidget(self.txt_k_color)
        lay_blur_color.addWidget(QLabel("Kernel"))
        lay_blur_color.addStretch()
        col_der.addLayout(lay_blur_color)

        col_der.addSpacing(15)

        lay_todas = QHBoxLayout()
        self.btn_todas = QPushButton("Todas")
        self.btn_todas.setFixedWidth(100)
        self.btn_todas.setStyleSheet("QPushButton { background-color: #555; border-radius: 10px; padding: 5px; } QPushButton:hover { background-color: #777; }")
        self.btn_todas.clicked.connect(self.seleccionar_todas)
        lbl_todas = QLabel("Se seleccionan todas las\ntransformaciones de imágenes")
        lbl_todas.setStyleSheet("font-size: 11px; color: #aaa;")
        lay_todas.addWidget(self.btn_todas)
        lay_todas.addWidget(lbl_todas)
        lay_todas.addStretch()
        col_der.addLayout(lay_todas)

        col_der.addStretch()

        lay_bottom = QHBoxLayout()
        
        self.btn_ejecutar = QPushButton("Ejecutar")
        self.btn_ejecutar.setFixedSize(120, 35)
        self.btn_ejecutar.setStyleSheet("QPushButton { background-color: #4CAF50; border-radius: 10px; font-weight: bold; } QPushButton:hover { background-color: #45a049; }")
        self.btn_ejecutar.clicked.connect(self.ejecutar_procesamiento)
        
        self.btn_acerca = QPushButton("Acerca de")
        self.btn_acerca.setFixedSize(100, 35)
        self.btn_acerca.setStyleSheet("QPushButton { background-color: #007BFF; border-radius: 10px; } QPushButton:hover { background-color: #0056b3; }")
        self.btn_acerca.clicked.connect(self.mostrar_acerca_de)

        self.lbl_logo = QLabel("LOGO TEC")
        self.lbl_logo.setAlignment(Qt.AlignCenter)
        self.lbl_logo.setFixedSize(60, 60)
        self.lbl_logo.setStyleSheet("background-color: white; color: #0033a0; font-weight: bold; border-radius: 5px;")
        
        ruta_logo = os.path.join(self.directorio_base, "logo.png")
        if os.path.exists(ruta_logo):
            pixmap = QPixmap(ruta_logo).scaled(60, 60, Qt.KeepAspectRatio, Qt.SmoothTransformation)
            self.lbl_logo.setPixmap(pixmap)

        lay_bottom.addWidget(self.btn_acerca)
        lay_bottom.addStretch()
        lay_bottom.addWidget(self.btn_ejecutar)
        lay_bottom.addStretch()
        lay_bottom.addWidget(self.lbl_logo)

        col_der.addLayout(lay_bottom)

        layout_principal.addLayout(col_izq, 1)
        layout_principal.addSpacing(30)
        layout_principal.addLayout(col_der, 2)
        
        self.setLayout(layout_principal)

    def seleccionar_todas(self):
        estado = True
        if (self.chk1.isChecked() and self.chk2.isChecked() and self.chk3.isChecked() and 
            self.chk4.isChecked() and self.chk5.isChecked() and self.chk6.isChecked()):
            estado = False
            
        self.chk1.setChecked(estado)
        self.chk2.setChecked(estado)
        self.chk3.setChecked(estado)
        self.chk4.setChecked(estado)
        self.chk5.setChecked(estado)
        self.chk6.setChecked(estado)

    def mostrar_acerca_de(self):
        dlg = QMessageBox(self)
        dlg.setWindowTitle("Acerca de")
        dlg.setText("<b>TC3003</b><br><br>"
                    "Tecnologico de monterrey Campus puebla<br>"
                    "Mayo 2026<br><br>"
                    "<i>Equipo:</i>"
                    "<ul>"
                    "<li>Francisco Antonio Lopez Ricardez</li>"
                    "<li>Alejandro Santana Moreno</li>"
                    "<li>Yahel Alejandro Jiménez Fernández </li>")
        dlg.setStyleSheet("QLabel { color: white; } QPushButton { background-color: #ddd; color: #000; }")
        dlg.exec_()

    def ejecutar_procesamiento(self):
        archivos = self.drop_zone.archivos
        if not archivos:
            QMessageBox.warning(self, "Atención", "No has cargado ninguna imagen.")
            return

        k_gris = self.txt_k_gris.text() or "27"
        k_color = self.txt_k_color.text() or "27"
        
        f1 = "1" if self.chk1.isChecked() else "0"
        f2 = "1" if self.chk2.isChecked() else "0"
        f3 = "1" if self.chk3.isChecked() else "0"
        f4 = "1" if self.chk4.isChecked() else "0"
        f5 = "1" if self.chk5.isChecked() else "0"
        f6 = "1" if self.chk6.isChecked() else "0"

        if all(f == "0" for f in [f1, f2, f3, f4, f5, f6]):
            QMessageBox.warning(self, "Atención", "Selecciona al menos una transformación.")
            return

        # Calcular tareas para la barra de progreso
        total_tareas = len(archivos)
        tareas_completadas = 0
        self.barra_progreso.setValue(0)
        self.txt_tiempo.setText("Procesando...")
        self.consola.clear()
        QApplication.processEvents()

        ruta_ejecutable = os.path.join(self.directorio_base, "main_mpi")
        ruta_hosts = os.path.join(self.directorio_base, "hosts_mpi")
        
        comando = [
            "mpirun", "-f", ruta_hosts, "-np", "6", 
            ruta_ejecutable, self.ruta_salida, 
            k_gris, k_color, f1, f2, f3, f4, f5, f6
        ] + archivos
        
        try:
            # Ejecución asíncrona para actualizar la barra y la consola en tiempo real
            proceso = subprocess.Popen(comando, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            
            tiempo_total = "Desconocido"
            
            # Leer la salida línea por línea mientras se ejecuta
            while True:
                linea = proceso.stdout.readline()
                if not linea and proceso.poll() is not None:
                    break
                if linea:
                    self.consola.append(linea.strip())
                    
                    # Detectar avance de tareas
                    if "Procesando la imagen" in linea:
                        tareas_completadas += 1
                        porcentaje = int((tareas_completadas / total_tareas) * 100)
                        self.barra_progreso.setValue(min(porcentaje, 100))
                    
                    if "TIEMPO_TOTAL:" in linea:
                        tiempo_total = linea.split(":")[1].strip() + " segundos"
                        
                    QApplication.processEvents() # Previene que la ventana se congele

            errores = proceso.stderr.read()
            if errores:
                self.consola.append("\n[ERRORES DEL SISTEMA]:\n" + errores)
            
            if proceso.returncode != 0:
                QMessageBox.critical(self, "Error del Sistema", "El programa en C falló. Revisa el monitor del clúster.")
                self.txt_tiempo.setText("Error")
                return

            self.txt_tiempo.setText(tiempo_total)
            self.barra_progreso.setValue(100)
            
        except FileNotFoundError:
            QMessageBox.critical(self, "Ejecutable no encontrado", 
                                 f"No se encontró el archivo compilado en:\n{ruta_ejecutable}\n\nAsegúrate de compilar el código en C en esa carpeta.")
            self.txt_tiempo.setText("Error")

if __name__ == '__main__':
    app = QApplication(sys.argv)
    ventana = AppProcesamiento()
    ventana.show()
    sys.exit(app.exec_())