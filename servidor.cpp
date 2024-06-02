#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <vector>

using namespace std;

const int ROWS = 6;
const int COLS = 7;
char board[ROWS][COLS];

void initialize_board() {
    for (int i = 0; i < ROWS; ++i)
        for (int j = 0; j < COLS; ++j)
            board[i][j] = '.';
}

void print_board() {
    for (int i = 0; i < ROWS; ++i) {
        for (int j = 0; j < COLS; ++j) {
            cout << board[i][j] << ' ';
        }
        cout << endl;
    }
    cout << endl;
}

string board_to_string() {
    string board_str;
    for (int i = 0; i < ROWS; ++i) {
        for (int j = 0; j < COLS; ++j) {
            board_str += board[i][j];
            board_str += ' ';
        }
        board_str += '\n';
    }
    return board_str;
}

bool drop_piece(int col, char piece) {
    for (int i = ROWS - 1; i >= 0; --i) {
        if (board[i][col] == '.') {
            board[i][col] = piece;
            return true;
        }
    }
    return false;
}

bool check_winner(char piece) {
    // Check horizontal
    for (int i = 0; i < ROWS; ++i)
        for (int j = 0; j < COLS - 3; ++j)
            if (board[i][j] == piece && board[i][j + 1] == piece && board[i][j + 2] == piece && board[i][j + 3] == piece)
                return true;

    // Check vertical
    for (int i = 0; i < ROWS - 3; ++i)
        for (int j = 0; j < COLS; ++j)
            if (board[i][j] == piece && board[i + 1][j] == piece && board[i + 2][j] == piece && board[i + 3][j] == piece)
                return true;

    // Check diagonal (bottom left to top right)
    for (int i = 3; i < ROWS; ++i)
        for (int j = 0; j < COLS - 3; ++j)
            if (board[i][j] == piece && board[i - 1][j + 1] == piece && board[i - 2][j + 2] == piece && board[i - 3][j + 3] == piece)
                return true;

    // Check diagonal (top left to bottom right)
    for (int i = 0; i < ROWS - 3; ++i)
        for (int j = 0; j < COLS - 3; ++j)
            if (board[i][j] == piece && board[i + 1][j + 1] == piece && board[i + 2][j + 2] == piece && board[i + 3][j + 3] == piece)
                return true;

    return false;
}

void play_game(int socket_player1, int socket_player2) {
    initialize_board();
    print_board();

    int current_player = 1;
    char pieces[] = {'X', 'O'};
    int socket_players[] = {socket_player1, socket_player2};
    char buffer[1024];

    while (true) {
        int socket_current = socket_players[current_player - 1];
        int socket_other = socket_players[2 - current_player];
        char current_piece = pieces[current_player - 1];

        string board_str = board_to_string();
        send(socket_current, board_str.c_str(), board_str.size(), 0);
        send(socket_other, board_str.c_str(), board_str.size(), 0);

        send(socket_current, "Tu turno\n", 9, 0);
        send(socket_other, "Espera tu turno\n", 16, 0);

        int bytes_received = recv(socket_current, buffer, 1024, 0);
        buffer[bytes_received] = '\0';

        int column = atoi(buffer);

        if (column < 0 || column >= COLS || !drop_piece(column, current_piece)) {
            send(socket_current, "Movimiento invalido\n", 20, 0);
            continue;
        }

        print_board();

        if (check_winner(current_piece)) {
            string board_str = board_to_string();
            send(socket_current, board_str.c_str(), board_str.size(), 0);
            send(socket_other, board_str.c_str(), board_str.size(), 0);
            send(socket_current, "Ganaste!\n", 9, 0);
            send(socket_other, "Perdiste!\n", 9, 0);
            break;
        }

        if (bytes_received == 0) {
            cout << "ConexiÃ³n cerrada\n";
            break;
        }

        current_player = 3 - current_player;
    }

    close(socket_player1);
    close(socket_player2);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        cout << "Uso: " << argv[0] << " <puerto>\n";
        return EXIT_FAILURE;
    }

    int port = atoi(argv[1]);
    int socket_server = 0;
    struct sockaddr_in direccionServidor, direccionCliente;

    if ((socket_server = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cout << "Error creando el socket\n";
        return EXIT_FAILURE;
    }

    memset(&direccionServidor, 0, sizeof(direccionServidor));
    direccionServidor.sin_family = AF_INET;
    direccionServidor.sin_addr.s_addr = htonl(INADDR_ANY);
    direccionServidor.sin_port = htons(port);

    if (bind(socket_server, (struct sockaddr *) &direccionServidor, sizeof(direccionServidor)) < 0) {
        cout << "Error en bind()\n";
        return EXIT_FAILURE;
    }

    if (listen(socket_server, 2) < 0) {
        cout << "Error en listen()\n";
        return EXIT_FAILURE;
    }

    cout << "Esperando conexiones...\n";
    socklen_t addr_size = sizeof(struct sockaddr_in);
    int socket_player1 = accept(socket_server, (struct sockaddr *)&direccionCliente, &addr_size);
    int socket_player2 = accept(socket_server, (struct sockaddr *)&direccionCliente, &addr_size);

    if (socket_player1 < 0 || socket_player2 < 0) {
        cout << "Error en accept()\n";
        return EXIT_FAILURE;
    }

    play_game(socket_player1, socket_player2);

    close(socket_server);
    return 0;
}