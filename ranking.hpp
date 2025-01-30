#ifndef RANKING_HPP
#define RANKING_HPP

#include <iostream>
#include <cstring>
#include <vector>


class Ranking {
    private:
        std::vector<std::string> nicknames;
        std::vector<int> wins;
        std::vector<int> loses;
        std::vector<int> draws;
        std::vector<int> points;
        std::vector<int> games;
        std::vector<bool> isConnected;
        std::vector<float> winRatio;
    public:
        
        Ranking() {};

        //return: 0 created new playeyer, 1 player with given nick exists but is not online so connect to this player
        // -1 player with given nickname existr but is online so decline this nickname
        int addPlayer(std::string nick) {
            int playerPos = -1;
            for (int i = 0;i < nicknames.size();i++) {
                if (nicknames.at(i) == nick) {
                    playerPos = i;
                    break;
                }
            }
            //player with given nickname not found
            if(playerPos==-1){
                nicknames.push_back(nick);
                wins.push_back(0);
                loses.push_back(0);
                draws.push_back(0);
                points.push_back(0);
                winRatio.push_back(0);
                isConnected.push_back(1);
                games.push_back(0);
                return 0;
            }
            else {
                if (isConnected[playerPos]) {
                    return -1;
                    
                }
                else {
                    isConnected[playerPos]=1;
                    return 1;
                    
                }
            }
            
        }

        void disconnectPlayer(std::string nick) {
            int playerPos;
            for (int i = 0;i < nicknames.size();i++) {
                if (nicknames.at(i) == nick) {
                    playerPos = i;
                    break;
                }
            }
            isConnected[playerPos] = 0;
        }

        void addWin(std::string nick) {
            int playerPos;
            for (int i = 0;i < nicknames.size();i++) {
                if (nicknames.at(i) == nick) {
                    playerPos = i;
                    break;
                }
            }
            wins[playerPos]++;
            updateRanks();
        }

        void addLose(std::string nick) {
            int playerPos;
            for (int i = 0;i < nicknames.size();i++) {
                if (nicknames.at(i) == nick) {
                    playerPos = i;
                    break;
                }
            }
            loses[playerPos]++;
            updateRanks();
        }

        void addDraw(std::string nick) {
            int playerPos;
            for (int i = 0;i < nicknames.size();i++) {
                if (nicknames.at(i) == nick) {
                    playerPos = i;
                    break;
                }
            }
            draws[playerPos]++;
            updateRanks();
        }

        void updateRanks() {
            for (int i = 0; i < nicknames.size();i++) {
                points[i] = wins[i] * 3 + draws[i];
                games[i] = wins[i] + loses[i] + draws[i];
                if (games[i] > 0) {
                    winRatio[i] = (static_cast<float>(wins[i]) / games[i]) * 100.0f;
                }
            }
        }

        std::string printRanking() {
            std::string printed;
            printed += "\nRanking:\n";
            printed += "Nick: \t\t\tGames:\t\tWins: \tLoses: \tDraws: \tPoints: \tWinRatio:\n";
            for (int i = 0; i < nicknames.size();i++) {
                printed += nicknames.at(i);
                printed += "\t\t\t";
                printed += std::to_string(games.at(i));
                printed += "\t\t";
                printed += std::to_string(wins.at(i));
                printed += "\t";
                printed += std::to_string(loses.at(i));
                printed += "\t";
                printed += std::to_string(draws.at(i));
                printed += "\t";
                printed += std::to_string(points.at(i));
                printed += "\t\t";
                printed += std::to_string(winRatio.at(i));
                printed.erase(printed.size() - 4);
                printed += " %\n";
            }
            printed += "\n";

            return printed;
        }

        std::string printOnline() {
            std::string printed;
            printed += "\nOnline players:\n";
            for (int i = 0;i < nicknames.size(); i++) {
                if (isConnected[i] == 1) {
                    printed += nicknames[i];
                    printed += ", ";
                }
            }
            printed += "\n\n";

            return printed;
        }
};

#endif