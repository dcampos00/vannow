# Gestión de Repositorios Duales: Versión Privada y Pública

Este documento describe las alternativas y el procedimiento técnico para mantener una versión **privada** (con toda la investigación de costos, riesgos y código en desarrollo) y una versión **pública** (con la documentación oficial y código estable) sin riesgo de filtrar información sensible.

---

## 1. El Riesgo de Historial (History Leakage)

En Git, aunque borres un archivo en el commit actual, **el archivo sigue existiendo en el historial de commits anteriores**. Si simplemente duplicas el repositorio actual o usas ramas y borras los archivos de investigación, cualquier persona en GitHub podrá ver tu historial de commits antiguos y descargar las evaluaciones de costos o riesgos de seguridad.

Para evitar esto, existen dos métodos principales:

---

## Método A: Dos Carpetas Separadas con Script de Sincronización (¡RECOMENDADO y 100% Seguro!)

Mantienes dos carpetas locales completamente independientes en tu ordenador:
1.  `/Users/daniel/Projects/vannow` (Tu repositorio **Privado** actual, donde trabajas).
2.  `/Users/daniel/Projects/vannow-public` (Tu repositorio **Público** limpio).

### Paso 1: Configurar el Repositorio Público
1.  Crea un nuevo repositorio vacío en GitHub (ej. `github.com/dcampos00/vannow-public`).
2.  Clónalo localmente en otra carpeta:
    ```bash
    git clone https://github.com/dcampos00/vannow-public.git /Users/daniel/Projects/vannow-public
    ```

### Paso 2: Crear el Script de Sincronización
En tu repositorio privado, creamos un script automático (`sync_public.sh`) que limpie el repositorio público, copie únicamente los archivos públicos, realice el commit y suba los cambios.

He creado el script de sincronización en tu carpeta raíz: [`sync_public.sh`](../sync_public.sh). Su lógica es:
*   Borra el contenido de la carpeta pública (excepto la carpeta `.git`).
*   Copia la estructura de `firmware/`, `hardware/`, el `README.md` y **únicamente** el documento oficial `docs/propuesta_proyecto.md`.
*   Deja fuera todos los documentos de análisis de costos, riesgos, seguridad y encoders.
*   Hace un commit automático en el repo público y hace `git push`.

---

## Método B: Repositorio Único con dos Remotos y Rama Limpia

Trabajas en una sola carpeta, pero configuras dos remotos en Git y mantienes una rama llamada `public` desligada de tu rama `main`.

### Paso 1: Agregar el segundo remoto
```bash
# Cambiar el remote actual para que sea el privado
git remote rename origin private

# Agregar el nuevo repositorio público de GitHub
git remote add public https://github.com/dcampos00/vannow-public.git
```

### Paso 2: Crear una rama pública huérfana (sin historial privado)
Para asegurarte de que el historial privado no se filtre al repositorio público:
```bash
# Crear una rama totalmente limpia y sin historial
git checkout --orphan public

# Borrar todos los archivos de investigación locales en esta rama
rm docs/evaluacion_costos_vanpi.md
rm docs/analisis_seguridad_rf.md
rm docs/estimacion_complejidad_tiempo.md
rm docs/atenuacion_remota.md
rm docs/control_encoder_rotativo.md
rm docs/opciones_encoders_bajo_perfil.md
rm docs/eficiencia_energetica.md

# Confirmar y subir a la rama main del repositorio público
git add .
git commit -m "Initial public release"
git push public public:main
```

### Inconveniente del Método B:
Debes tener extremo cuidado. Si por error ejecutas un `git merge main` estando en la rama `public` y luego haces push, **subirás todo el historial de archivos privados** al repositorio público de GitHub.

---

## Recomendación

El **Método A** es el estándar de oro en seguridad. Al estar físicamente en carpetas distintas, es imposible cometer un error de comando de Git que exponga tus archivos privados a internet.
