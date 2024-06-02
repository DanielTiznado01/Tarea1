#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <iostream>

using namespace std;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        cout << "Uso: " << argv[0] << " <direccion IP> <puerto>\n";
        return EXIT_FAILURE;
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);
    int socket_cliente;
    struct sockaddr_in direccionServidor;

    if ((socket_cliente = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cout << "Error creando el socket\n";
        return EXIT_FAILURE;
    }

    memset(&direccionServidor, 0, sizeof(direccionServidor));
    direccionServidor.sin_family = AF_INET;
    direccionServidor.sin_port = htons(port);

    if (inet_pton(AF_INET, server_ip, &direccionServidor.sin_addr) <= 0) {
        cout << "Dirección IP inválida\n";
        return EXIT_FAILURE;
    }

    if (connect(socket_cliente, (struct sockaddr *)&direccionServidor, sizeof(direccionServidor)) < 0) {
        cout << "Error en connect()\n";
        return EXIT_FAILURE;
    }

    char buffer[1024];
    while (true) {
        int bytes_received = recv(socket_cliente, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            cout << "Conexión cerrada por el servidor\n";
            break;
        }

        buffer[bytes_received] = '\0';
        cout << buffer;

        if (strstr(buffer, "Tu turno")) {
            cout << "Seleccionar columna: ";
            int col;
            cin >> col;
            string col_str = to_string(col);
            send(socket_cliente, col_str.c_str(), col_str.size(), 0);
        }
    }

    close(socket_cliente);
    return 0;
}
