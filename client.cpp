#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define TCP_PORT 8080
#define UDP_PORT 9090
#define BUFFER_SIZE 1024
#define BROADCAST_IP "255.255.255.255"

class Client {
private:
    int tcp_sock, udp_sock;
    struct sockaddr_in server_address, udp_address;

    bool receiveFromServer() {
        char buffer[BUFFER_SIZE];
        while (true) {
            memset(buffer, 0, BUFFER_SIZE);
            int valread = read(tcp_sock, buffer, BUFFER_SIZE);
            if (valread > 0) {
                std::cout << "\nSerwer:\n" << buffer << std::endl;
            }
            else if (valread == 0) {
                std::cout << "Serwer zakończył połączenie.\n";
                return 1;
            }
            else {
                std::cerr << "Błąd odbioru danych z serwera\n";
                break;
            }
        }
        return 0;
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

        const int max_retries = 5;
        const int retry_interval = 2; // Czas w sekundach między próbami
        bool server_found = false;

        for (int attempt = 0; attempt < max_retries; ++attempt) {
            std::string broadcast_message = "DISCOVER_SERVER";
            sendto(udp_sock, broadcast_message.c_str(), broadcast_message.size(), 0,
                (struct sockaddr*)&udp_address, sizeof(udp_address));

            char buffer[BUFFER_SIZE];
            struct sockaddr_in server_response;
            socklen_t server_len = sizeof(server_response);

            // Ustaw timeout na gniazdo UDP
            struct timeval tv;
            tv.tv_sec = retry_interval;
            tv.tv_usec = 0;
            setsockopt(udp_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

            int recv_len = recvfrom(udp_sock, buffer, BUFFER_SIZE, 0,
                (struct sockaddr*)&server_response, &server_len);
            if (recv_len > 0) {
                std::cout << "Otrzymano odpowiedź od serwera: " << inet_ntoa(server_response.sin_addr) << "\n";
                server_address.sin_family = AF_INET;
                server_address.sin_port = htons(TCP_PORT);
                server_address.sin_addr = server_response.sin_addr;
                server_found = true;
                break;
            }
            else {
                std::cout << "Brak odpowiedzi od serwera. Próba " << (attempt + 1) << " z " << max_retries << ".\n";
            }
        }

        if (!server_found) {
            std::cerr << "Nie znaleziono serwera po " << max_retries << " próbach. Kończę działanie.\n";
            exit(EXIT_FAILURE);
        }




        // Setup TCP connection
        if ((tcp_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("TCP Socket creation error");
            exit(EXIT_FAILURE);
        }

        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(TCP_PORT);
        server_address.sin_addr = server_address.sin_addr;

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
            std::cout << "Serwer: " << buffer << std::endl;
        }
        else {
            std::cerr << "Błąd odbioru danych od serwera\n";
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
            if (receiveFromServer()) {
                std::cout << "Kończę połączenie...\n";
                kill(pid, SIGKILL); // Zakończ proces potomny
            }
        }
        else {
            //
            // Proces rodzicielski: wysyłanie wiadomości do serwera
            //
            while (true) {
                std::string input;
                std::getline(std::cin, input);

                // client can send only exit or numer 1-9
                if (input == "exit") {
                    std::cout << "Kończę połączenie...\n";
                    kill(pid, SIGKILL); // Zakończ proces potomny
                    break;
                }
                else if (input[0]>48&&input[0]<58&&input.size() == 1) {
                    send(tcp_sock, input.c_str(), input.size(), 0);
                }
            }

            waitpid(pid, nullptr, 0);

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
