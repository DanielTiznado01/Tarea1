#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <pthread.h>
#include <vector>
#include <cstdlib>
#include <ctime>

using namespace std;

const int ROWS = 6;
const int COLS = 7;
const int BUFFER_SIZE = 1024;

struct Game {
    int socket_player;
    char board[ROWS][COLS];
    bool is_player_start; 
    string client_ip;
    int client_port;
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
    
    for (int i = 0; i < ROWS; ++i)
        for (int j = 0; j < COLS - 3; ++j)
            if (board[i][j] == piece && board[i][j + 1] == piece && board[i][j + 2] == piece && board[i][j + 3] == piece)
                return true;

    
    for (int i = 0; i < ROWS - 3; ++i)
        for (int j = 0; j < COLS; ++j)
            if (board[i][j] == piece && board[i + 1][j] == piece && board[i + 2][j] == piece && board[i + 3][j] == piece)
                return true;

    
    for (int i = 3; i < ROWS; ++i)
        for (int j = 0; j < COLS - 3; ++j)
            if (board[i][j] == piece && board[i - 1][j + 1] == piece && board[i - 2][j + 2] == piece && board[i - 3][j + 3] == piece)
                return true;

    
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

    int current_player = game->is_player_start ? 2 : 1; 
    char pieces[] = {'S', 'C'};
    char buffer[BUFFER_SIZE];

    cout << "Juego [" << game->client_ip << ":" << game->client_port << "]: ";
    if (game->is_player_start) {
        cout << "inicia juego el cliente." << endl;
    } else {
        cout << "inicia juego el servidor." << endl;
    }

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
        if (current_player == 2) {
            int bytes_received = recv(socket_player, buffer, BUFFER_SIZE, 0);
            if (bytes_received <= 0) {
                cout << "Conexión cerrada\n";
                break;
            }
            buffer[bytes_received] = '\0';
            column = atoi(buffer) - 1;
            cout << "Juego [" << game->client_ip << ":" << game->client_port << "]: cliente juega columna " << (column + 1) << "." << endl;
        } else { 
            column = rand() % COLS;
            cout << "Juego [" << game->client_ip << ":" << game->client_port << "]: servidor juega columna " << (column + 1) << "." << endl;
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
                cout << "Juego [" << game->client_ip << ":" << game->client_port << "]: gana cliente." << endl;
            } else {
                send(socket_player, "Perdiste!\n", 9, 0);
                cout << "Juego [" << game->client_ip << ":" << game->client_port << "]: gana servidor." << endl;
            }
            break;
        }

        current_player = 3 - current_player; 
    }

    cout << "Juego [" << game->client_ip << ":" << game->client_port << "]: fin del juego." << endl;

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

    srand(time(nullptr)); 

    cout << "Esperando conexiones...\n";

    while (true) {
        socklen_t addr_size = sizeof(struct sockaddr_in);
        int socket_player = accept(socket_server, (struct sockaddr *)&direccionCliente, &addr_size);
        if (socket_player < 0) {
            cout << "Error en accept()\n";
            continue;
        }

        string client_ip = inet_ntoa(direccionCliente.sin_addr);
        int client_port = ntohs(direccionCliente.sin_port);

        cout << "Juego nuevo[" << client_ip << ":" << client_port << "]" << endl;

        Game* game = new Game();
        game->socket_player = socket_player;
        game->is_player_start = rand() % 2; 
        game->client_ip = client_ip;
        game->client_port = client_port;

        pthread_t thread_id;
        pthread_create(&thread_id, nullptr, handle_game, (void*)game);
        pthread_detach(thread_id); 
    }

    close(socket_server);
    return 0;
}
