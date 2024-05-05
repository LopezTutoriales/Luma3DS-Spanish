# Luma3DS

*"Custom Firmware" para Nintendo 3DS*

## Lo que es
**Luma3DS** es un programa que parchea y reimplementa partes importantes del software que se ejecuta en todos los modelos de la familia de consolas Nintendo 3DS.

Su objetivo es mejorar enormemente la experiencia del usuario y dar soporte a la 3DS mucho más allá de su fin de vida. Sus características son:

* Soporte de primera clase para homebrew 3DSX
* Un menú superpuesto llamado "Rosalina" (activable con <kbd>L+Abajo+Select</kbd> por defecto), permitiendo, entre muchas cosas, tomar capturas de pantalla mientras estás en el juego
* Eliminación de restricciones como el bloqueo de región
* Configuración de idioma por juego, redirección de ruta de contenido de asset (LayeredFS), plugins de juegos...
* Un código auxiliar GDB completo que permite depurar software (tanto de homebrew como de sistema)
* ... y mucho más!

Luma3DS requiere un exploit persistente en todo el sistema, como [boot9strap](https://github.com/SciresM/boot9strap) para ejecutarse.

## Compilacion

Para compilar Luma3DS, necesitas lo siguiente:
* git
* DevkitARM + libctru actualizado
* [makerom](https://github.com/jakcron/Project_CTR) en PATH
* [firmtool](https://github.com/TuxSH/firmtool) instalado

El "boot.firm" producido está destinado a ser copiado a la raíz de su tarjeta SD para su uso con Boot9Strap.

## Configuración / Uso / Funciones
Ver https://github.com/LumaTeam/Luma3DS/wiki (necesita reelaboración)

## Créditos
Ver https://github.com/LumaTeam/Luma3DS/wiki/Credits (necesita reelaboración)

## Licencia
Este software tiene licencia según los términos de la GPLv3. Puede encontrar una copia de la licencia en el archivo LICENSE.txt.

Los archivos en el stub de GDB tienen una licencia triple como MIT o "GPLv2 o cualquier versión posterior", en cuyo caso se especifica en el encabezado del archivo.

Al contribuir a este repositorio, acepta otorgar licencias de sus cambios a los propietarios del proyecto.
