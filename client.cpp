#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <csignal>

#define TCP_PORT 8080
#define UDP_PORT 9090
#define BUFFER_SIZE 1024
#define BROADCAST_IP "255.255.255.255"

class Client {
private:
    int tcp_sock, udp_sock;
    struct sockaddr_in server_address, udp_address;

    void receiveFromServer() {
        char buffer[BUFFER_SIZE];
        while (true) {
            memset(buffer, 0, BUFFER_SIZE);
            int valread = read(tcp_sock, buffer, BUFFER_SIZE);
            if (valread > 0) {
                std::cout << "\nSerwer:\n" << buffer << std::endl;
            }
            else if (valread == 0) {
                std::cout << "Serwer zakończył połączenie.\n";
                break;
            }
            else {
                std::cerr << "Błąd odbioru danych z serwera\n";
                break;
            }
        }
        exit(0); // Zakończ proces odbierający
    }

public:
    Client() {
        // UDP socket setup
        if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            perror("UDP Socket creation error");
            exit(EXIT_FAILURE);
        }

        int broadcast = 1;
        if (setsockopt(udp_sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
            perror("Set broadcast failed");
            close(udp_sock);
            exit(EXIT_FAILURE);
        }

        udp_address.sin_family = AF_INET;
        udp_address.sin_port = htons(UDP_PORT);
        udp_address.sin_addr.s_addr = inet_addr(BROADCAST_IP);

        // Send broadcast
        std::string broadcast_message = "DISCOVER_SERVER";
        sendto(udp_sock, broadcast_message.c_str(), broadcast_message.size(), 0,
            (struct sockaddr*)&udp_address, sizeof(udp_address));

        // Receive response
        char buffer[BUFFER_SIZE];
        struct sockaddr_in server_response;
        socklen_t server_len = sizeof(server_response);
        recvfrom(udp_sock, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&server_response, &server_len);
        std::cout << "Otrzymano odpowiedź od serwera: " << inet_ntoa(server_response.sin_addr) << "\n";

        // Setup TCP connection
        if ((tcp_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("TCP Socket creation error");
            exit(EXIT_FAILURE);
        }

        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(TCP_PORT);
        server_address.sin_addr = server_response.sin_addr;

        if (connect(tcp_sock, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
            perror("Connection failed");
            exit(EXIT_FAILURE);
        }
    }

    void sendNickname(const std::string& nickname) {
        send(tcp_sock, nickname.c_str(), nickname.size(), 0);
        char buffer[BUFFER_SIZE] = { 0 };
        int valread = read(tcp_sock, buffer, BUFFER_SIZE);
        if (valread > 0) {
            std::cout << "\nSerwer:\n" << buffer << std::endl;
        }
        else {
            std::cerr << "Błąd odbioru danych od serwera\n";
        }
    }

    void displayStatsAndStartGame() {
        char buffer[BUFFER_SIZE];
        while (true) {
            memset(buffer, 0, BUFFER_SIZE);
            std::cout << "Aby rozpocząć grę, wpisz 'start': ";
            std::string input;
            std::getline(std::cin, input);

            if (input == "start") {
                send(tcp_sock, input.c_str(), input.size(), 0);
                int valread = read(tcp_sock, buffer, BUFFER_SIZE);
                if (valread > 0) {
                    std::cout << "\nSerwer:\n" << buffer << std::endl;
                }
                break;
            } else {
                std::cout << "Nieprawidłowa komenda. Wpisz 'start', aby rozpocząć grę.\n";
            }
        }
    }

    void startCommunication() {
        pid_t pid = fork();

        if (pid < 0) {
            perror("Fork error");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            // Proces potomny: odbieranie wiadomości od serwera
            receiveFromServer();
        }
        else {
            // Proces rodzicielski: wyświetlanie panelu statystyk i rozpoczęcie gry
            displayStatsAndStartGame();

            while (true) {
                std::string input;
                std::getline(std::cin, input);

                if (input == "exit") {
                    std::cout << "Kończę połączenie...\n";
                    kill(pid, SIGKILL); // Zakończ proces potomny
                    break;
                }

                send(tcp_sock, input.c_str(), input.size(), 0);
            }

            waitpid(pid, nullptr, 0); // Poczekaj na zakończenie procesu potomnego
        }
    }

    ~Client() {
        close(tcp_sock);
        close(udp_sock);
    }
};

int main() {
    Client client;

    std::string nickname;
    std::cout << "Podaj swój nickname: ";
    std::getline(std::cin, nickname);

    client.sendNickname(nickname);
    client.startCommunication();

    return 0;
}
