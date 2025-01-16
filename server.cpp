#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define TCP_PORT 8080
#define UDP_PORT 9090
#define BUFFER_SIZE 1024

void handleClient(int client_socket) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    // Deklaracja zmiennej nickname poza blokiem
    std::string nickname;

    // Odczyt nicku gracza
    int valread = read(client_socket, buffer, BUFFER_SIZE);
    if (valread > 0) {
        buffer[valread] = '\0';
        nickname = buffer; // Zapis nicku do zmiennej
        std::cout << "Dodano gracza: " << nickname << "\n";

        std::string message = "Nickname zaakceptowany!\n";
        send(client_socket, message.c_str(), message.size(), 0);
    }
    else {
        std::cerr << "Błąd odbioru danych od klienta\n";
        close(client_socket);
        exit(0);
    }

    while (true) {
        // Wysyłanie wiadomości do klienta
        std::string serverMessage = "To jest wiadomość z serwera\n";
        send(client_socket, serverMessage.c_str(), serverMessage.size(), 0);

        // Odczyt danych od klienta
        memset(buffer, 0, BUFFER_SIZE);
        valread = read(client_socket, buffer, BUFFER_SIZE);
        if (valread > 0) {
            buffer[valread] = '\0';
            std::cout << "Otrzymano od " << nickname << ": " << buffer << "\n";
        }
        else if (valread == 0) {
            std::cout << "Klient " << nickname << " rozłączył się.\n";
            break;
        }
        else {
            std::cerr << "Błąd odbioru danych od klienta\n";
            break;
        }

        sleep(1); // Krótkie opóźnienie dla symulacji
    }

    close(client_socket);
    exit(0);
}

void handleZombieProcesses(int signal) {
    while (waitpid(-1, nullptr, WNOHANG) > 0);
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

    std::cout << "Serwer UDP nasłuchuje na porcie " << UDP_PORT << "...\n";

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int len = recvfrom(udp_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &client_len);
        if (len > 0) {
            std::cout << "Otrzymano zapytanie broadcastowe od " << inet_ntoa(client_addr.sin_addr) << "\n";
            std::string response = "ACK";
            sendto(udp_fd, response.c_str(), response.size(), 0, (struct sockaddr*)&client_addr, client_len);
        }
    }
}

int main() {
    signal(SIGCHLD, handleZombieProcesses);

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

    std::cout << "Serwer TCP nasłuchuje na porcie " << TCP_PORT << "...\n";

    int udp_fd;
    if ((udp_fd = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
        perror("UDP Socket failed");
        exit(EXIT_FAILURE);
    }

    pid_t udp_process = fork();
    if (udp_process == 0) {
        startUDPServer(udp_fd);
        exit(0);
    }
    else if (udp_process < 0) {
        perror("Fork failed for UDP server");
        close(udp_fd);
        close(tcp_server_fd);
        exit(EXIT_FAILURE);
    }

    while (true) {
        struct sockaddr_in client_address;
        socklen_t addrlen = sizeof(client_address);

        int new_socket = accept(tcp_server_fd, (struct sockaddr*)&client_address, &addrlen);
        if (new_socket < 0) {
            perror("Accept failed");
            continue;
        }

        std::cout << "Nowe połączenie od klienta " << inet_ntoa(client_address.sin_addr) << ":" << ntohs(client_address.sin_port) << "\n";

        pid_t pid = fork();
        if (pid == 0) {
            close(tcp_server_fd);
            handleClient(new_socket);
        }
        else if (pid > 0) {
            close(new_socket);
        }
        else {
            perror("Fork failed");
        }
    }

    close(tcp_server_fd);
    close(udp_fd);
    return 0;
}
