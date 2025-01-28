#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <functional>
#include "game.hpp"

#define TCP_PORT 8080
#define UDP_PORT 9090
#define BUFFER_SIZE 1024

std::vector<Player> players;
std::mutex playerMutex;
std::condition_variable playerCV;

void handleGame(Player p1, Player p2) {
    std::string message = "All players connected, starting game...\n";
    send(p1.socket, message.c_str(), message.size(), 0);
    send(p2.socket, message.c_str(), message.size(), 0);

    TicTacToeGame game(p1, p2);
    game.run();

    message = "Closing game...\n";
    send(p1.socket, message.c_str(), message.size(), 0);
    send(p2.socket, message.c_str(), message.size(), 0);
    close(p1.socket);
    close(p2.socket);
}

void handleClientCommunication(int client_socket, std::string nickname) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    std::string clientMessage;
    int valread = read(client_socket, buffer, BUFFER_SIZE);
    if (valread > 0) {
        buffer[valread] = '\0';
        clientMessage = buffer;
        if (clientMessage == "1")
        {
            std::cout << "Player : " << nickname << "wants to start game!" << "\n";
            std::string message;

            // Add player to the list
            {
                std::lock_guard<std::mutex> lock(playerMutex);
                players.emplace_back(client_socket, nickname);
            }
            // Notify that a new player is added
            playerCV.notify_all();

            std::cout << "Game lobby has: " << players.size() << " players \n";

            if (players.size() % 2 == 0)
            {
                std::cout << "\nAll players connected\n\n";
                message = "All players connected, starting game...\n";
                // Move the last two players to local variables.
                auto p1 = std::move(players.back());
                players.pop_back();
                auto p2 = std::move(players.back());
                players.pop_back();

                // Now clear the original vector.
                players.clear();

                // Capture p1 and p2 by copy in the thread's lambda.
                std::thread gameThread([p1, p2]() mutable {
                    handleGame(p1, p2);
                });
                gameThread.detach();

            }
            else {
                std::string message = "Waiting for another player...\n";
                send(client_socket, message.c_str(), message.size(), 0);
            }


        }
        else {
            close(client_socket);
            return;
        }

    }
    else {
        std::cerr << "Error receiving data from client\n";
        close(client_socket);
        return;
    }

}

void handleClient(int client_socket) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    std::string nickname;

    // Read nickname from the client
    int valread = read(client_socket, buffer, BUFFER_SIZE);
    if (valread > 0) {
        buffer[valread] = '\0';
        nickname = buffer;
        std::cout << "Added player: " << nickname << "\n";

        std::string message = "Nickname accepted!\n";
        send(client_socket, message.c_str(), message.size(), 0);

    } else {
        std::cerr << "Error receiving data from client\n";
        close(client_socket);
        return;
    }

    handleClientCommunication(client_socket, nickname);

    // Additional handling (game logic, etc.) can go here
}


void startUDPServer(int udp_fd) {
    struct sockaddr_in udp_address, client_addr;
    char buffer[BUFFER_SIZE];
    socklen_t client_len = sizeof(client_addr);

    udp_address.sin_family = AF_INET;
    udp_address.sin_addr.s_addr = INADDR_ANY;
    udp_address.sin_port = htons(UDP_PORT);

    if (bind(udp_fd, (struct sockaddr*)&udp_address, sizeof(udp_address)) < 0) {
        perror("UDP Bind failed");
        close(udp_fd);
        exit(EXIT_FAILURE);
    }

    std::cout << "UDP server listening on port " << UDP_PORT << "...\n";

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int len = recvfrom(udp_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &client_len);
        if (len > 0) {
            std::cout << "Received broadcast request from " << inet_ntoa(client_addr.sin_addr) << "\n";
            std::string response = "ACK";
            sendto(udp_fd, response.c_str(), response.size(), 0, (struct sockaddr*)&client_addr, client_len);
        }
    }
}

int main() {
    int tcp_server_fd;
    struct sockaddr_in tcp_address;

    if ((tcp_server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(tcp_server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("Setsockopt failed");
        close(tcp_server_fd);
        exit(EXIT_FAILURE);
    }

    tcp_address.sin_family = AF_INET;
    tcp_address.sin_addr.s_addr = INADDR_ANY;
    tcp_address.sin_port = htons(TCP_PORT);

    if (bind(tcp_server_fd, (struct sockaddr*)&tcp_address, sizeof(tcp_address)) < 0) {
        perror("Bind failed");
        close(tcp_server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(tcp_server_fd, 10) < 0) {
        perror("Listen failed");
        close(tcp_server_fd);
        exit(EXIT_FAILURE);
    }

    std::cout << "TCP server listening on port " << TCP_PORT << "...\n";

    int udp_fd;
    if ((udp_fd = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
        perror("UDP Socket failed");
        exit(EXIT_FAILURE);
    }

    std::thread udpThread([udp_fd]() {
        startUDPServer(udp_fd);
    });
    udpThread.detach();

    while (true) {
        struct sockaddr_in client_address;
        socklen_t addrlen = sizeof(client_address);

        int new_socket = accept(tcp_server_fd, (struct sockaddr*)&client_address, &addrlen);
        if (new_socket < 0) {
            perror("Accept failed");
            continue;
        }

        std::cout << "New connection from client " << inet_ntoa(client_address.sin_addr) << ":" << ntohs(client_address.sin_port) << "\n";

        // Spawn a new thread to handle the client
        std::thread clientThread([new_socket]() {
            handleClient(new_socket);
        });
        clientThread.detach();
    }

    close(tcp_server_fd);
    close(udp_fd);
    return 0;
}
