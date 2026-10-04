# Keylogger-Basico-Educativo
# Keylogger Educativo - Trabajo Final de Seguridad Informática

> ⚠️ **ADVERTENCIA LEGAL**
> Este proyecto es un keylogger desarrollado **exclusivamente con fines
> educativos** como parte del trabajo final del curso de Seguridad
> Informática de [Nombre de tu institución].
>
> **Está estrictamente prohibido su uso en sistemas sin autorización
> explícita y por escrito del propietario.** La ejecución de este software
> en equipos ajenos constituye un delito en la mayoría de jurisdicciones.
>
> El autor no se hace responsable del uso indebido por parte de terceros.
> Este repositorio se publica únicamente como referencia académica.

## Descripción

Keylogger desarrollado en C++ para Windows que:
- Captura pulsaciones de teclado mediante `SetWindowsHookEx` (hook de bajo nivel).
- Registra la ventana activa en el momento de cada pulsación.
- Rota los archivos de log cada 30 segundos.
- Exfiltra los logs cifrados por HTTPS a un webhook de Discord cada 30 segundos.
- Mueve los archivos enviados a una carpeta `enviados/`.

## Uso legítimo

Este proyecto está pensado para:
- **Investigación académica** sobre técnicas de malware.
- **Pruebas en entornos controlados** (máquinas virtuales aisladas).
- **Auditorías de seguridad autorizadas por escrito**.

## NO usar para

- Espiar a personas sin su consentimiento.
- Robar credenciales o información de terceros.
- Ejecutar en equipos ajenos sin permiso explícito.
- Cualquier actividad que viole la legislación vigente.

## Requisitos para compilar

- Kali Linux (o Debian/Ubuntu) como entorno de cross-compilación.
- `x86_64-w64-mingw32-g++` (MinGW-w64).
- `libcurl` compilado para MinGW con soporte SSL.
- Alternativa: compilar directamente en Windows con MSYS2.

## Compilación

[Comando exacto de compilación documentado]

## Configuración

1. Crear un archivo `webhook.txt` en el mismo directorio que el `.exe`
   con la URL del webhook de Discord:
   
## Modo de uso 
1. Una vez clonado el repositorio descarga en la misma carpeta el cacert.perm con: https://curl.se/ca/cacert.pem
2. El webhook_example.txt está con un link de referencia, copia tu propio link de tu propio webhook y pegalo ahí
3. Compila el código y ya! :D

