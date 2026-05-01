# 📚 Wiki del Proyecto

Esta carpeta contiene la **documentación para la Wiki de GitHub** del proyecto de Interfaz Gráfica de Procesamiento Paralelo de Imágenes.

## 📖 Cómo usarla

### Opción 1: Subir a GitHub (Recomendado)

1. **Crear un repositorio en GitHub** (si no existe)
   
2. **Inicializar la wiki:**
   - En GitHub, ir a: `Settings` → `Features` → Habilitar ☑️ **Wiki**

3. **Clonar la wiki como repositorio:**
   ```bash
   git clone https://github.com/TU_USUARIO/TU_REPO.wiki.git
   ```

4. **Copiar estos archivos:**
   ```bash
   cp wiki/* ../TU_REPO.wiki/
   cd ../TU_REPO.wiki/
   git add .
   git commit -m "Initial wiki import"
   git push
   ```

5. **¡Acceder a la wiki:**
   - URL: `https://github.com/TU_USUARIO/TU_REPO/wiki`

### Opción 2: Vista local

- Abrir `Home.md` en un editor Markdown
- Los links `[[Página]]` pueden no funcionar localmente, pero la estructura está clara

---

## 📁 Estructura de archivos

```
wiki/
├── _Sidebar.md                 ← Barra lateral de navegación (GitHub)
├── _Footer.md                  ← Pie de página (GitHub)
├── Home.md                     ← Página de inicio
├── 1-Objetivos.md              ← Metas del experimento
├── 2-Contexto-Experimental.md  ← Hardware y metodología
├── 3-Resultados-y-Datos.md     ← Tabla consolidada
├── 4-Análisis-Detallado.md     ← Análisis por dispositivo
├── 5-Conclusiones.md           ← Recomendaciones
├── 6-Arquitectura.md           ← Funcionamiento de la UI
└── 7-Guía-Instalación.md       ← Compilación y uso
```

---

## 🎯 Contenido resumido

| Página | Propósito |
|--------|-----------|
| **Home** | Introducción y tabla de contenidos |
| **Objetivos** | Preguntas de investigación |
| **Contexto** | Hardware y programa utilizado |
| **Resultados** | Datos consolidados con tabla y hallazgos |
| **Análisis** | Interpretación por laptop |
| **Conclusiones** | Recomendaciones de nodos maestro/secundarios |
| **Arquitectura** | Explicación de UI ↔ Backend |
| **Instalación** | Pasos de compilación multiplataforma |

---

## ✨ Características de GitHub Wiki

### Archivos especiales

- **`_Sidebar.md`**: Barra lateral con navegación personalizada
- **`_Footer.md`**: Pie de página en todas las páginas

### Links internos

```markdown
[[Página]]      → Link a "Página.md"
[Ver más](6-Arquitectura)  → Link estándar markdown
```

### Formateo

- ✅ Markdown estándar
- ✅ Tablas
- ✅ Emojis
- ✅ Código con syntax highlighting
- ✅ Listas y sub-listas

---

## 📝 Notas

- Cada página tiene navegación al pie (← Anterior | Siguiente →)
- Todos los links están organizados jerárquicamente
- La sidebar proporciona acceso rápido a todas las secciones
- Las imágenes pueden agregarse si se suben a la carpeta `wiki/images/`

---

## 🔧 Personalización

Para adaptar la wiki a tu repositorio:

1. **Home.md** - Actualizar nombre del proyecto
2. **_Sidebar.md** - Cambiar links si renombras páginas
3. **_Footer.md** - Actualizar fecha y versión

---

**¡La wiki está lista para GitHub!** 🚀
