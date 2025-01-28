#ifndef GAME_HPP
#define GAME_HPP

#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>     // for close()
#include <sys/socket.h> // for send(), recv()
#include "player.hpp"


class TicTacToeGame {
public:
    TicTacToeGame(Player p1, Player p2)
        : player1_(p1), player2_(p2)
    {
        // Initialize the board with empty spaces
        board_.assign(9, ' ');
    }

    void run() {
        // Decide who is 'O' and who is 'X'.
        // For simplicity, let's say player1_ is 'O' and goes first, 
        // and player2_ is 'X' and goes second.
        char p1Mark = 'O';
        char p2Mark = 'X';

        // Inform the players of their marks
        sendToPlayer(player1_, "You are 'O'. You move first.\n");
        sendToPlayer(player2_, "You are 'X'. You move second.\n");
        broadcastBoard();

        // Game loop
        bool gameRunning = true;
        int currentTurn = 0;  // 0 for player1_, 1 for player2_

        while (gameRunning) {
            // Current player's mark and socket
            Player& currentPlayer = (currentTurn == 0) ? player1_ : player2_;
            char currentMark = (currentTurn == 0) ? p1Mark : p2Mark;

            
            // Ask current player to make a move
            sendToPlayer(currentPlayer, "Your move (1-9): ");

            // Read move
            int chosenPos = getMoveFromPlayer(currentPlayer);
            if (chosenPos == -1) {
                // If there's an error in reading, let's close the game.
                sendToPlayer(player1_, "Communication error. Closing game.\n");
                sendToPlayer(player2_, "Communication error. Closing game.\n");
                break;
            }

            // Validate move
            if (chosenPos < 1 || chosenPos > 9 || board_[chosenPos - 1] != ' ') {
                // Invalid move; ask to retry
                sendToPlayer(currentPlayer, "Invalid move. Try again.\n");
                sendBoard(currentPlayer);
                continue;
            }

            // Place the mark
            board_[chosenPos - 1] = currentMark;

            // Send updated board to both players
            broadcastBoard();

            // Check winner
            if (checkWinner(currentMark)) {
                std::string winMsg = "Player [" + currentPlayer.nickname 
                                     + "] (" + currentMark + ") wins!\n";
                sendToPlayer(player1_, winMsg);
                sendToPlayer(player2_, winMsg);
                gameRunning = false;
            } 
            else if (checkDraw()) {
                std::string drawMsg = "It's a draw!\n";
                sendToPlayer(player1_, drawMsg);
                sendToPlayer(player2_, drawMsg);
                gameRunning = false;
            }

            // Switch turn
            currentTurn = (currentTurn + 1) % 2;
        }
    }

private:
    Player player1_;
    Player player2_;
    std::vector<char> board_; // 9 cells representing the tic-tac-toe board

    // Helper function to send a string message to a specific player
    void sendToPlayer(const Player& player, const std::string& message) {
        send(player.socket, message.c_str(), message.size(), 0);
    }

    // Helper function to read the move from player's socket
    // Returns -1 on error or the parsed move [1..9] on success.
    int getMoveFromPlayer(const Player& player) {
        char buffer[128];
        memset(buffer, 0, sizeof(buffer));

        int bytesReceived = recv(player.socket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) {
            // Error or socket closed
            return -1;
        }

        // Null-terminate and parse
        buffer[bytesReceived] = '\0';
        std::string moveStr(buffer);

        try {
            return std::stoi(moveStr);
        } catch (...) {
            // Invalid conversion
            return -1;
        }
    }

    // Broadcast the board to both players
    void broadcastBoard() {
        std::string boardString = getBoardString();
        sendToPlayer(player1_, boardString);
        sendToPlayer(player2_, boardString);
    }

    void sendBoard(Player p) {
        std::string boardString = getBoardString();
        sendToPlayer(p, boardString);
    }

    // Returns a formatted string of the board
    std::string getBoardString() {
        // Example ASCII board representation
        //  1 | 2 | 3
        // ---+---+---
        //  4 | 5 | 6
        // ---+---+---
        //  7 | 8 | 9
        //
        // Replaced by the actual X/O or ' ' in each position
        std::string result;
        for (int i = 0; i < 9; i++) {
            if ( i == 0 || i == 3 || i == 6)
            {
                result += " ";
            }
            
            result.push_back(board_[i] == ' ' ? ' ' : board_[i]);
            if ((i + 1) % 3 == 0 && i < 8) {
                result += (i == 2 || i == 5) ? "\n---+---+---\n" : "\n";
            } else if ((i + 1) % 3 != 0) {
                result += " | ";
            }
        }
        result += "\n\n";
        return result;
    }

    // Check for a winner with current mark
    bool checkWinner(char mark) {
        // All winning combinations
        static int winCombos[8][3] = {
            {0,1,2}, {3,4,5}, {6,7,8}, // rows
            {0,3,6}, {1,4,7}, {2,5,8}, // cols
            {0,4,8}, {2,4,6}           // diagonals
        };

        for (auto & combo : winCombos) {
            if (board_[combo[0]] == mark &&
                board_[combo[1]] == mark &&
                board_[combo[2]] == mark) {
                return true;
            }
        }

        return false;
    }

    // Check if board is fully occupied (draw)
    bool checkDraw() {
        for (char cell : board_) {
            if (cell == ' ') {
                return false;
            }
        }
        return true;
    }
};


#endif