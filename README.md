# ESP8266 Reflow Oven Controller
*Instrucciones en español mas abajo*
## Introduction

This is a simple PID controller with a web interface for a reflow oven. It requires a MAX31856, a K-type thermocouple, a MOC3041 optotriac, a suitable triac for your mains voltage, and a ESP-12E module. 

### Software

The Arduino-based firmware is included. It requres tzapu's [WiFiManager](https://github.com/tzapu/WiFiManager) to set up. It also includes ArduinoOTA for remote firmware upgrades.

### Hardware

The KiCAD files are available too, but this is a simple circuit that can, obviously, be built out of standard arduino modules. Instead of the triac and optotriac, a SSR (Solid State Relay) can be used. A mechanical relay cannot be used as this circuit switches the load several times per second for accurate temperature control.

---

## Introducción

Este es un controlador PID muy simple, con una interfaz web, para controlar un horno de soldadura por refusion. Necesita un MAX31856, una termocupla tipo K, un optotriac MOC3041, un triac adecuado para tu voltaje de linea, y un modulo ESP-12E

### Software

Se incluye el firmware basado en Arduino. Necesita la libreria [WiFiManager](https://github.com/tzapu/WiFiManager) de tzapu para la configuracion. Tambien incluye ArduinoOTA para actualizaciones remotas.

### Hardware

También se incluyen los archivos de KiCAD, pero este es un circuito tan simple que obviamente se puede armar con modulos Arduino comunes. En lugar del triac y optotriac, se puede usar un modulo SSR. No es posible usar un relay mecánico ya que el circuito conmuta la salida varias veces por segundo para un control preciso de la temperatura.
