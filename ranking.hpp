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
    public:
        
        Ranking() {};

        void addPlayer(std::string nick) {
            nicknames.push_back(nick);
            wins.push_back(0);
            loses.push_back(0);
            draws.push_back(0);
            points.push_back(0);
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
           // updateRanks();
        }

        std::string printRanking() {
            std::string printed;
            printed += "\nRanking:\n";
            printed += "Nick: \t\tWins: \tLoses: \tDraws: \tPoints:\n";
            for (int i = 0; i < nicknames.size();i++) {
                printed += nicknames.at(i);
                printed += "\t\t";
                printed += std::to_string(wins.at(i));
                printed += "\t";
                printed += std::to_string(loses.at(i));
                printed += "\t";
                printed += std::to_string(draws.at(i));
                printed += "\t";
                printed += std::to_string(points.at(i));
                printed += "\n";
            }

            return printed;
        }
};

#endif