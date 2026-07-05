#!/bin/bash

# ==============================================================================
# SCRIPT DE SINCRONIZACIÓN: VanNOW Privado -> Público
# ==============================================================================
# Este script copia selectivamente solo los archivos públicos y firmware
# estables desde tu repositorio de desarrollo privado al repositorio público,
# previniendo la fuga de documentos de investigación, costos o riesgos.

# Rutas de los repositorios locales
PRIVATE_REPO="/Users/daniel/Projects/vannow"
PUBLIC_REPO="/Users/daniel/Projects/vannow-public"

# Validar que exista la carpeta del repositorio público
if [ ! -d "$PUBLIC_REPO" ]; then
    echo "ERROR: No se encuentra el directorio del repositorio público en: $PUBLIC_REPO"
    echo "Por favor, crea la carpeta y clona tu repositorio público de GitHub allí primero."
    exit 1
fi

echo "Iniciando sincronización de VanNOW público..."

# 1. Limpiar el repositorio público (excepto la base de datos de git)
echo "Limpiando archivos antiguos en el repositorio público..."
find "$PUBLIC_REPO" -maxdepth 1 -not -name '.' -not -name '.git' -not -name '.gitignore' -exec rm -rf {} +
find "$PUBLIC_REPO/docs" -mindepth 1 -exec rm -rf {} + 2>/dev/null
find "$PUBLIC_REPO/firmware" -mindepth 1 -exec rm -rf {} + 2>/dev/null
find "$PUBLIC_REPO/hardware" -mindepth 1 -exec rm -rf {} + 2>/dev/null

# 2. Crear estructura de carpetas en el repositorio público si no existen
mkdir -p "$PUBLIC_REPO/docs"
mkdir -p "$PUBLIC_REPO/firmware"
mkdir -p "$PUBLIC_REPO/hardware"

# 3. Copiar archivos públicos seleccionados
echo "Copiando archivos autorizados al repositorio público..."

# Raíz
cp "$PRIVATE_REPO/README.md" "$PUBLIC_REPO/"

# Documentación oficial seleccionada (Sólo la propuesta de diseño inicial)
cp "$PRIVATE_REPO/docs/propuesta_proyecto.md" "$PUBLIC_REPO/docs/"

# Firmware completo (Central y Remoto)
cp -R "$PRIVATE_REPO/firmware/"* "$PUBLIC_REPO/firmware/"

# Hardware completo (Atopile)
cp -R "$PRIVATE_REPO/hardware/"* "$PUBLIC_REPO/hardware/"

echo "Archivos copiados con éxito."

# 4. Git commit y push en el repositorio público
echo "Subiendo cambios a GitHub Público..."
cd "$PUBLIC_REPO" || exit

# Verificar si hay cambios antes de hacer commit
if [ -n "$(git status --porcelain)" ]; then
    git add .
    git commit -m "Sync: Sincronización oficial del firmware y documentación de VanNOW - $(date '+%Y-%m-%d %H:%M:%S')"
    git push origin main
    echo "¡Repositorio público actualizado y subido correctamente!"
else
    echo "No hay cambios detectados. El repositorio público ya está al día."
fi

# Volver a la carpeta de trabajo
cd "$PRIVATE_REPO" || exit
