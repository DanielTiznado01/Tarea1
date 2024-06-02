#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <pthread.h>
#include <vector>
#include <cstdlib> // Para la función rand() y srand()
#include <ctime>   // Para la función time()

using namespace std;

const int ROWS = 6;
const int COLS = 7;
const int BUFFER_SIZE = 1024;

struct Game {
    int socket_player;
    char board[ROWS][COLS];
    bool is_player_start; // Nuevo campo para indicar quién empieza
};

void initialize_board(char board[ROWS][COLS]) {
    for (int i = 0; i < ROWS; ++i)
        for (int j = 0; j < COLS; ++j)
            board[i][j] = '.';
}

void print_board(const char board[ROWS][COLS]) {
    cout << "TABLERO" << endl;
    for (int i = 0; i < ROWS; ++i) {
        for (int j = 0; j < COLS; ++j) {
            cout << board[i][j] << ' ';
        }
        cout << endl;
    }
    cout << "-------------" << endl;
    cout << "1 2 3 4 5 6 7" << endl;
    cout << endl;
}

string board_to_string(const char board[ROWS][COLS]) {
    string board_str = "TABLERO\n";
    for (int i = 0; i < ROWS; ++i) {
        for (int j = 0; j < COLS; ++j) {
            board_str += board[i][j];
            board_str += ' ';
        }
        board_str += '\n';
    }
    board_str += "-------------\n";
    board_str += "1 2 3 4 5 6 7\n";
    return board_str;
}

bool drop_piece(char board[ROWS][COLS], int col, char piece) {
    for (int i = ROWS - 1; i >= 0; --i) {
        if (board[i][col] == '.') {
            board[i][col] = piece;
            return true;
        }
    }
    return false;
}

bool check_winner(const char board[ROWS][COLS], char piece) {
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

void* handle_game(void* arg) {
    Game* game = (Game*)arg;
    int socket_player = game->socket_player;
    char (*board)[COLS] = game->board;

    initialize_board(board);
    print_board(board);

    int current_player = game->is_player_start ? 2 : 1; // Cliente empieza si is_player_start es true
    char pieces[] = {'S', 'C'};
    char buffer[BUFFER_SIZE];

    while (true) {
        char current_piece = pieces[current_player - 1];

        string board_str = board_to_string(board);
        send(socket_player, board_str.c_str(), board_str.size(), 0);

        if (current_player == 2) {
            send(socket_player, "Tu turno\n", 9, 0);
        } else {
            send(socket_player, "Espera tu turno\n", 16, 0);
        }

        int column;
        if (current_player == 2) { // Movimiento del cliente
            int bytes_received = recv(socket_player, buffer, BUFFER_SIZE, 0);
            if (bytes_received <= 0) {
                cout << "Conexión cerrada\n";
                break;
            }
            buffer[bytes_received] = '\0';
            column = atoi(buffer) - 1;
        } else { // Movimiento del servidor
            column = rand() % COLS;
        }

        if (column < 0 || column >= COLS || !drop_piece(board, column, current_piece)) {
            if (current_player == 2) {
                send(socket_player, "Movimiento inválido\n", 20, 0);
            }
            continue;
        }

        print_board(board);

        if (check_winner(board, current_piece)) {
            string board_str = board_to_string(board);
            send(socket_player, board_str.c_str(), board_str.size(), 0);
            if (current_player == 2) {
                send(socket_player, "Ganaste!\n", 9, 0);
                cout << "Juego: gana cliente.\n";
            } else {
                send(socket_player, "Perdiste!\n", 9, 0);
                cout << "Juego: gana servidor.\n";
            }
            break;
        }

        current_player = 3 - current_player; // Alterna entre 1 y 2
    }

    close(socket_player);
    delete game;
    return nullptr;
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

    if (listen(socket_server, 10) < 0) {
        cout << "Error en listen()\n";
        return EXIT_FAILURE;
    }

    srand(time(nullptr)); // Inicializa la semilla para rand()

    cout << "Esperando conexiones...\n";

    while (true) {
        socklen_t addr_size = sizeof(struct sockaddr_in);
        int socket_player = accept(socket_server, (struct sockaddr *)&direccionCliente, &addr_size);
        if (socket_player < 0) {
            cout << "Error en accept()\n";
            continue;
        }

        cout << "Juego nuevo[" << inet_ntoa(direccionCliente.sin_addr) << ":" << ntohs(direccionCliente.sin_port) << "]\n";

        Game* game = new Game();
        game->socket_player = socket_player;
        game->is_player_start = rand() % 2; // Decide aleatoriamente si el cliente empieza

        pthread_t thread_id;
        pthread_create(&thread_id, nullptr, handle_game, (void*)game);
        pthread_detach(thread_id);
    }

    close(socket_server);
    return 0;
}