# WattControl – Taller de mecanizado

Gestor de energía para un taller de mecanizado, en C++ con programación orientada a objetos.
EA2 – Programación Orientada a Objetos, IU Digital de Antioquia (2026-2).
Autor: José Luis Alzate Quiroz.

## Negocio
Taller de mecanizado con tres cargas:
- Iluminación (crítica, 1,5 kW): nunca se apaga.
- Torno CNC (aplazable, 7,5 kW).
- Compresor (aplazable, 3,0 kW).

Potencia contratada: 10 kW. El gestor decide cada hora qué cargas encender
según el precio de la energía, la hora pico y el nivel de los embalses.

## Compilar y ejecutar
    g++ wattcontrol.cpp -o wattcontrol
    ./wattcontrol        (en Windows: wattcontrol.exe)

## Clases y relaciones
- Herencia: CargaCritica y CargaAplazable heredan de Carga (clase abstracta).
- Polimorfismo: `debeEncender()` es virtual puro y se invoca con `Carga*`.
- Composición: Negocio tiene un Medidor.
- Agregación: Negocio agrupa Cargas con `agregarCarga()`.
- Asociación: Negocio usa un Mercado.

Diagrama: `diagrama_clases.png` (editable en `diagrama_clases_wattcontrol.drawio`).
