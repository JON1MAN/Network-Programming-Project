#include <iostream>
#include <string>
#include <cstring>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
#include <arpa/inet.h>

#define TCP_PORT 8080
#define BUFFER_SIZE 1024

struct Player {
    int socket;
    std::string nickname;
};

std::unordered_map<std::string, std::tuple<int, int, int>> playerStats = {
    {"Mario", {8, 2, 5}},
    {"Luigi", {10, 3, 4}},
    {"Peach", {15, 1, 2}}
};

std::vector<Player> players;
std::mutex playerMutex;
std::condition_variable playerCV;

std::string getPlayerStats(const std::string& nickname) {
    if (playerStats.find(nickname) != playerStats.end()) {
        auto [wins, ties, losses] = playerStats[nickname];
        return "Player: " + nickname + "\nWins: " + std::to_string(wins) +
               "\nTies: " + std::to_string(ties) +
               "\nLoses: " + std::to_string(losses) +
               "\nTo start game send: start\n";
    } else {
        return "Player: " + nickname +
               "\nNo statistics found.\nTo start game send: start\n";
    }
}

void handleClient(int client_socket) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    // Read player's nickname
    int valread = read(client_socket, buffer, BUFFER_SIZE);
    if (valread <= 0) {
        std::cerr << "Failed to receive nickname from client.\n";
        close(client_socket);
        return;
    }

    buffer[valread] = '\0';
    std::string nickname(buffer);
    std::cout << "Player connected: " << nickname << "\n";

    // Send player statistics
    std::string stats = getPlayerStats(nickname);
    send(client_socket, stats.c_str(), stats.size(), 0);

    // Wait for "start" command
    memset(buffer, 0, BUFFER_SIZE);
    valread = read(client_socket, buffer, BUFFER_SIZE);
    if (valread > 0) {
        buffer[valread] = '\0';
        if (std::string(buffer) == "start") {
            std::cout << "Player " << nickname << " is ready to start.\n";

            {
                // Add the player to the global list
                std::unique_lock<std::mutex> lock(playerMutex);
                players.push_back({client_socket, nickname});
                playerCV.notify_all();
            }

            // Wait for game to start
            {
                std::unique_lock<std::mutex> lock(playerMutex);
                playerCV.wait(lock, []() { return players.size() >= 2; });
            }

            if (players.size() >= 2) {
                int player1Socket = players[0].socket;
                int player2Socket = players[1].socket;

                std::string messageToPlayer1 = "Game started! You are 'X'. Your opponent is: " + players[1].nickname + "\n";
                std::string messageToPlayer2 = "Game started! You are 'O'. Your opponent is: " + players[0].nickname + "\n";

                send(player1Socket, messageToPlayer1.c_str(), messageToPlayer1.size(), 0);
                send(player2Socket, messageToPlayer2.c_str(), messageToPlayer2.size(), 0);

                // Remove players from the list after starting the game
                players.clear();
            }
        } else {
            std::cerr << "Unexpected command from player: " << buffer << "\n";
        }
    }

    close(client_socket);
}

int main() {
    int server_fd;
    struct sockaddr_in address;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("Setsockopt failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(TCP_PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << TCP_PORT << "...\n";

    while (true) {
        struct sockaddr_in client_address;
        socklen_t addrlen = sizeof(client_address);

        int new_socket = accept(server_fd, (struct sockaddr*)&client_address, &addrlen);
        if (new_socket < 0) {
            perror("Accept failed");
            continue;
        }

        std::cout << "New connection from " << inet_ntoa(client_address.sin_addr) << ":" << ntohs(client_address.sin_port) << "\n";

        std::thread(handleClient, new_socket).detach();
    }

    close(server_fd);
    return 0;
}
