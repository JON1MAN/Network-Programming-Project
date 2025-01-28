#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <iostream>
#include <string>

class Player {
public:
    int socket;
    std::string nickname;
    
    Player(int s, std::string n) : socket(s), nickname(n) {}
};

#endif