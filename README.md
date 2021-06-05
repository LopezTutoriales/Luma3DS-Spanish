# Luma3DS
* "Firmware personalizado" (N)3DS a prueba de novatos *

### Lo que es
**Luma3DS** es un programa para parchear el software del sistema de las (nuevas) consolas portátiles Nintendo (2) 3DS "sobre la marcha", agregando funciones como configuraciones de idioma por juego, capacidades de depuración para desarrolladores y eliminando restricciones impuestas por Nintendo como el bloqueo de la región.

También le permite ejecutar contenido no autorizado ("homebrew") eliminando los controles de firma.
Para usarlo, necesitará una consola capaz de ejecutar software homebrew en el procesador Arm9.

Desde la versión 8.0, Luma3DS tiene su propio menú en el juego, que se puede activar con <kbd> L + Abajo + Select </kbd> (consulte las [notas de la versión](https://github.com/LumaTeam/Luma3DS/releases/tag/v8.0)).

#
### Compilación
* Requisitos previos
    1. git
    2. [makerom](https://github.com/jakcron/Project_CTR) en PATH
    3. [firmtool](https://github.com/TuxSH/firmtool)
    4. DevkitARM + libctru actualizado
1. Clone el repositorio con `git clone https://github.com/LumaTeam/Luma3DS.git`
2. Ejecute `make`.

    El "boot.firm" producido está destinado a ser copiado a la raíz de su tarjeta SD para su uso con Boot9Strap.

#
### Configuración / Uso / Funciones
Ver https://github.com/LumaTeam/Luma3DS/wiki

#
### Créditos
Ver https://github.com/LumaTeam/Luma3DS/wiki/Credits

#
### Licencia
Este software tiene licencia según los términos de la GPLv3. Puede encontrar una copia de la licencia en el archivo LICENSE.txt.

En cambio, los archivos en el stub de GDB tienen una licencia triple como MIT o "GPLv2 o cualquier versión posterior", en cuyo caso se especifica en el encabezado del archivo.
