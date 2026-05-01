import sys
import os
import subprocess
from PyQt5.QtWidgets import (QApplication, QWidget, QVBoxLayout, QHBoxLayout, 
                             QLabel, QPushButton, QCheckBox, QLineEdit, 
                             QFileDialog, QMessageBox, QGridLayout)
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
        self.resize(650, 400)
        self.setStyleSheet("background-color: #1e1e1e; color: #eeeeee; font-family: Arial;")
        
        # 1. OBTENER EL DIRECTORIO REAL DEL SCRIPT (Ignora dónde esté la terminal)
        self.directorio_base = os.path.dirname(os.path.abspath(__file__))
        
        # 2. CREAR LA RUTA DE RESULTADOS BASADA EN EL DIRECTORIO DEL SCRIPT
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
        
        col_izq.addSpacing(20)
        
        self.lbl_tiempo = QLabel("Tiempo de ejecución:")
        self.txt_tiempo = QLineEdit()
        self.txt_tiempo.setReadOnly(True)
        self.txt_tiempo.setStyleSheet("background-color: #333; border: 1px solid #555; padding: 5px;")
        col_izq.addWidget(self.lbl_tiempo)
        col_izq.addWidget(self.txt_tiempo)
        
        col_izq.addSpacing(10)
        
        self.lbl_ruta = QLabel("Ruta de archivos (Resultados):")
        # Mostrar la ruta absoluta de salida
        self.txt_ruta = QLineEdit(self.ruta_salida)
        self.txt_ruta.setReadOnly(True)
        self.txt_ruta.setStyleSheet("background-color: #333; border: 1px solid #555; padding: 5px; font-size: 11px;")
        col_izq.addWidget(self.lbl_ruta)
        col_izq.addWidget(self.txt_ruta)
        
        col_izq.addStretch()

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

        # Contenedor para Desenfoque Gris
        lay_blur_gris = QHBoxLayout()
        self.chk5 = QCheckBox("5- Desenfoque escala de grises")
        self.chk5.setStyleSheet(estilo_check)
        self.txt_k_gris = QLineEdit("27")
        self.txt_k_gris.setValidator(QIntValidator(1, 999)) # Solo enteros
        self.txt_k_gris.setFixedWidth(50)
        self.txt_k_gris.setStyleSheet("background-color: #333; border: 1px solid #555;")
        lay_blur_gris.addWidget(self.chk5)
        lay_blur_gris.addWidget(self.txt_k_gris)
        lay_blur_gris.addWidget(QLabel("Kernel"))
        lay_blur_gris.addStretch()
        col_der.addLayout(lay_blur_gris)

        # Contenedor para Desenfoque Color
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

        # Botón "Todas"
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

        # Botones Inferiores y Logo
        lay_bottom = QHBoxLayout()
        
        self.btn_ejecutar = QPushButton("Ejecutar")
        self.btn_ejecutar.setFixedSize(120, 35)
        self.btn_ejecutar.setStyleSheet("QPushButton { background-color: #4CAF50; border-radius: 10px; font-weight: bold; } QPushButton:hover { background-color: #45a049; }")
        self.btn_ejecutar.clicked.connect(self.ejecutar_procesamiento)
        
        self.btn_acerca = QPushButton("Acerca de")
        self.btn_acerca.setFixedSize(100, 35)
        self.btn_acerca.setStyleSheet("QPushButton { background-color: #007BFF; border-radius: 10px; } QPushButton:hover { background-color: #0056b3; }")
        self.btn_acerca.clicked.connect(self.mostrar_acerca_de)

        # 3. RUTA ABSOLUTA PARA EL LOGO
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

        # Ensamblar columnas
        layout_principal.addLayout(col_izq, 1)
        layout_principal.addSpacing(30)
        layout_principal.addLayout(col_der, 2)
        
        self.setLayout(layout_principal)

    # ================= FUNCIONES LÓGICAS =================
    def seleccionar_todas(self):
        estado = True
        # Si todas están marcadas, las desmarca para ser más intuitivo
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
        dlg.setText("<b>Proyecto de Procesamiento de Imágenes Paralelo</b><br><br>"
                    "Desarrollado para optimizar tiempos de ejecución utilizando OpenMP y C.<br>"
                    "Maneja imágenes .bmp y aplica filtros a nivel de bits.<br><br>"
                    "<i>Desarrollado por: Emmanuel Torres Rios</i>")
        dlg.setStyleSheet("QLabel { color: #000; } QPushButton { background-color: #ddd; color: #000; }")
        dlg.exec_()

    def ejecutar_procesamiento(self):
        archivos = self.drop_zone.archivos
        if not archivos:
            QMessageBox.warning(self, "Atención", "No has cargado ninguna imagen.")
            return

        # Obtener valores
        k_gris = self.txt_k_gris.text() or "27"
        k_color = self.txt_k_color.text() or "27"
        
        # Banderas (1 si está seleccionado, 0 si no)
        f1 = "1" if self.chk1.isChecked() else "0"
        f2 = "1" if self.chk2.isChecked() else "0"
        f3 = "1" if self.chk3.isChecked() else "0"
        f4 = "1" if self.chk4.isChecked() else "0"
        f5 = "1" if self.chk5.isChecked() else "0"
        f6 = "1" if self.chk6.isChecked() else "0"

        if all(f == "0" for f in [f1, f2, f3, f4, f5, f6]):
            QMessageBox.warning(self, "Atención", "Selecciona al menos una transformación.")
            return

        self.txt_tiempo.setText("Procesando...")
        QApplication.processEvents() # Fuerza a la interfaz a actualizar el texto

        # 4. RUTA ABSOLUTA PARA EL EJECUTABLE EN C (Cross-platform)
        nombre_ejecutable = "main.exe" if os.name == 'nt' else "main"
        ruta_ejecutable = os.path.join(self.directorio_base, nombre_ejecutable)
        
        comando = [ruta_ejecutable, self.ruta_salida, k_gris, k_color, f1, f2, f3, f4, f5, f6] + archivos

        try:
            # Ejecutar el backend en C
            resultado = subprocess.run(comando, capture_output=True, text=True)
            
            if resultado.returncode != 0:
                QMessageBox.critical(self, "Error del Sistema", f"El programa en C falló.\n{resultado.stderr}")
                self.txt_tiempo.setText("Error")
                return

            # Extraer el tiempo de la salida de texto (stdout)
            salida = resultado.stdout
            tiempo = "Desconocido"
            for linea in salida.split('\n'):
                if "TIEMPO_TOTAL:" in linea:
                    tiempo = linea.split(":")[1].strip() + " segundos"
            
            self.txt_tiempo.setText(tiempo)
            
        except FileNotFoundError:
            QMessageBox.critical(self, "Ejecutable no encontrado", 
                                 f"No se encontró el archivo compilado en:\n{ruta_ejecutable}\n\nAsegúrate de compilar el código en C en esa carpeta.")
            self.txt_tiempo.setText("Error")

if __name__ == '__main__':
    app = QApplication(sys.argv)
    ventana = AppProcesamiento()
    ventana.show()
    sys.exit(app.exec_())