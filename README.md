## Trivial Torrent (Conexión UDP)

Este repositorio contiene una implementación básica de Trivial Torrent utilizando UDP (User Datagram Protocol) para la comunicación entre clientes y servidor.

### Descripción

Trivial Torrent es una aplicación que permite la transferencia de archivos de manera simplificada entre un cliente y un servidor. En esta versión, se utiliza UDP para la comunicación, lo que proporciona una comunicación más ligera y sin conexión.

### Funcionalidades

- **Transferencia de Archivos**: Los clientes pueden solicitar bloques de archivos al servidor y recibirlos a través de la red.
- **Servidor de Torrent**: El servidor escucha las solicitudes de los clientes y responde proporcionando los bloques solicitados, si están disponibles.

### Tecnologías Utilizadas

- **C**: Lenguaje de programación utilizado para la implementación del cliente y servidor.
- **UDP (User Datagram Protocol)**: Protocolo de comunicación utilizado para la transferencia de datos entre cliente y servidor.
- **Sockets**: Utilizados para la creación y gestión de conexiones de red.

### Instrucciones de Uso

1. Clona este repositorio en tu máquina local.
2. Compila el cliente y el servidor utilizando un compilador C de tu elección (por ejemplo, gcc):

    ```bash
    gcc -o client client.c
    gcc -o server server.c
    ```

3. Ejecuta el servidor en una terminal:

    ```bash
    ./server [puerto] [ruta_del_archivo]
    ```

    - `[puerto]`: Puerto en el que el servidor escuchará las solicitudes de los clientes.
    - `[ruta_del_archivo]`: Ruta del archivo del cual se servirán los bloques.

4. Ejecuta el cliente en otra terminal para solicitar bloques del servidor:

    ```bash
    ./client [dirección_ip_servidor] [puerto_servidor] [número_de_bloque]
    ```

    - `[dirección_ip_servidor]`: Dirección IP del servidor.
    - `[puerto_servidor]`: Puerto en el que el servidor está escuchando.
    - `[número_de_bloque]`: Número del bloque que se solicitará al servidor.

### Notas Adicionales

- Este proyecto es una implementación básica y puede no ser adecuado para entornos de producción.
- La comunicación UDP es más ligera que la comunicación TCP, pero no garantiza la entrega de los paquetes ni el orden de llegada.
- Se recomienda utilizar este proyecto con fines educativos o de prueba.

¡Disfruta explorando Trivial Torrent con conexión UDP! Si tienes alguna pregunta o sugerencia, no dudes en comunicarte con el desarrollador. ¡Feliz transferencia de archivos! 🚀📡
