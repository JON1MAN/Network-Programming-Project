﻿#include <iostream>
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

Ranking ranking;

void handleClientCommunication(int client_socket, std::string nickname);

void handleGame(Player p1, Player p2) {
    std::string message = "All players connected, starting game...\n";
    send(p1.socket, message.c_str(), message.size(), 0);
    send(p2.socket, message.c_str(), message.size(), 0);

    TicTacToeGame game(p1, p2, ranking);
    game.run();

    std::thread client1Thread([p1]() {
        handleClientCommunication(p1.socket,p1.nickname);
        });
    client1Thread.detach();

    std::thread client2Thread([p2]() {
        handleClientCommunication(p2.socket,p2.nickname);
        });
    client2Thread.detach();
}

void sendRanking(int socket, std::string nickname) {
    std::string message = ranking.printRanking();
    send(socket, message.c_str(), message.size(), 0);
    std::thread clientThread([socket, nickname]() {
        handleClientCommunication(socket, nickname);
        });
    clientThread.detach();
}

void sendOnline(int socket, std::string nickname) {
    std::string message = ranking.printOnline();
    send(socket, message.c_str(), message.size(), 0);
    std::thread clientThread([socket, nickname]() {
        handleClientCommunication(socket, nickname);
        });
    clientThread.detach();
}

void sendLobby(int socket, std::string nickname) {
    std::string message;
    if (players.size() > 0) {
        message = "\nPlayer in lobby: ";
        message += players[0].nickname;
        message += "\n\n";
    }
    else {
        message = "\nLobby is empty.\n\n";
    }
    send(socket, message.c_str(), message.size(), 0);
    std::thread clientThread([socket, nickname]() {
        handleClientCommunication(socket, nickname);
        });
    clientThread.detach();
}

void handleClientCommunication(int client_socket, std::string nickname) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    std::string clientMessage;
    
    std::string message = "Welcome to the game!\n1.Start game\n2.Check ranking\n3.Check online players\n4.Check lobby\n5.exit\n";
    send(client_socket, message.c_str(), message.size(), 0);


    int valread; valread = read(client_socket, buffer, BUFFER_SIZE);
    if (valread > 0) {
        buffer[valread] = '\0';
        clientMessage = buffer;
        std::cout << "Player : " << nickname << " sends: " << clientMessage << "\n";
        if (clientMessage == "1")
        {
            std::cout << "Player : " << nickname << " wants to start game!" << "\n";


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
                message = "\nWaiting for another player...\n\n";
                send(client_socket, message.c_str(), message.size(), 0);
            }


        }
        else if (clientMessage == "2") {
            sendRanking(client_socket, nickname);
        }
        else if (clientMessage == "3") {
            sendOnline(client_socket, nickname);
        }
        else if (clientMessage == "4") {
            sendLobby(client_socket, nickname);
        }
        else {
            std::cout << "Disconnecting player:" << nickname << "\n";
            close(client_socket);
            ranking.disconnectPlayer(nickname);
            return;
        }

    }
    else {
        std::cerr << "Error receiving data from client\n";
        close(client_socket);
        ranking.disconnectPlayer(nickname);
        return;
    }

}

void handleClient(int client_socket) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    std::string nickname;
    std::string message;
    bool nicknameAccepted = 0;
    int valread;
    // Read nickname from the client
    while (!nicknameAccepted) {
        valread = read(client_socket, buffer, BUFFER_SIZE);
        if (valread > 0) {
            buffer[valread] = '\0';
            nickname = buffer;
            if (ranking.addPlayer(nickname) >= 0) {
                std::cout << "Added player: " << nickname << "\n";
                message = "\nNickname accepted!\n\n";
                send(client_socket, message.c_str(), message.size(), 0);
                nicknameAccepted = 1;

            }
            else {
                message = "Nickname not accepted!\nTry again:";
                send(client_socket, message.c_str(), message.size(), 0);
            }
        }
        else {
            std::cerr << "Error receiving data from client\n";
            close(client_socket);
            return;
        }
    }

    handleClientCommunication(client_socket, nickname);

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
