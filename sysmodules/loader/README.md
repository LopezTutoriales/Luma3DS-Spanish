3DS Loader Replacement
======================

Esta es una implementación de código abierto del módulo del sistema `loader` de 3DS, con
características adicionales. El objetivo actual del proyecto es proporcionar un buen
punto de entrada para parchear módulos de 3DS.

## Guía
En este momento, esto puede servir como un reemplazo de código abierto para el loader incorporado.
Hay soporte adicional para parchear cualquier ejecutable después de cargarlo, pero
antes de que comience. Por ejemplo, puede parchear `menu` para omitir las comprobaciones de región y
hacer que el juego libre de la región se inicie directamente desde el menú de inicio. También hay
soporte para lectura SDMC (no encontrado en la implementación del loader original) que
significa que los parches se pueden cargar desde la tarjeta SD. En definitiva, habría
un sistema de parches que admite la carga sencilla de parches desde la tarjeta SD.

## Build (Crear)
Necesita un entorno de compilación 3DS que funcione con una copia bastante reciente de devkitARM,
ctrulib y makerom. Si ve algún error en el proceso de compilación, es probable que
que está utilizando una versión anterior.

Actualmente, no hay soporte para la creación de FIRM, por lo que debe realizar algunos pasos
a mano. Primero, debe agregar relleno para asegurarse de que el NCCH sea correcto
tamaño para colocar como reemplazo. Una manera hacky es
[este parche](http://pastebin.com/nyKXLnNh) que agrega datos basura. Jugar
con el valor de tamaño para que el NCCH tenga exactamente el mismo tamaño que el
encontrado en su volcado FIRM descifrado.

Una vez que tenga un NCCH del tamaño correcto, simplemente reemplácelo en su FIRMA descifrada
y encuentre una forma de iniciarlo (por ejemplo, con ReiNAND).
