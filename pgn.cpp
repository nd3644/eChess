#include "pgn.h"
#include <iostream>
#include <fstream>
#include <sstream>

PGN::PGN() {
    iCurrentMove = 1;
    iTotalMoves = 0;
}

PGN::~PGN() {
}


std::string PGN::GetMove(int i) {
    return moveList[i];
}

int PGN::GetTotalMoves() {
    return iTotalMoves;
}

void PGN::Load(std::string sfilename) {
    std::ifstream in(sfilename);
    if(in.fail()) {
        std::cout << "couldn't load " << sfilename << std::endl;
        return;
    }

    std::string sLine;
    std::string moveText;
    while(!in.eof()) {
        std::getline(in, sLine);


        // line is metadata
        if(sLine[0] == '[') {
            std::string symbol, token;

            int tokenStart = 0;
            // get the symbol
            for(int i = 1;i < sLine.length();i++) {
                symbol += sLine[i];
                if(sLine[i] == ' ') {
                    tokenStart = i+2;
                    break;
                }
            }
            // get the token
            for(int i = tokenStart;i < sLine.length();i++) {
                if(sLine[i] == '\"') {
                    break;
                }
                token += sLine[i];
            }
            std::cout << "symbol : " << symbol << " " << token << std::endl;
        }
        else {
            moveText += sLine + " ";
        }
    }
    in.close();

    // clean movetext
    for(int i = 0;i < moveText.length();i++) {
        if(moveText[i] == '{') {
            for(int count = 0;count < 256;count++) {
                moveText.erase(moveText.begin() + i);
                if(moveText[i] == '}') {
                    moveText.erase(moveText.begin() + i);
                    break;
                }
            }
        }
    }

    iTotalMoves = 0;

    // Parse the movetext
    std::stringstream ss(moveText);
    while(!ss.eof()) {
        std::string str;

        // white .
        ss >> str;
        ss >> str;
        moveList.push_back(str);

        iTotalMoves++;

        // black ...
        ss >> str;
        ss >> str;
        moveList.push_back(str);

        iTotalMoves++;
    }
    iTotalMoves--;

    for(unsigned int i = 0;i < moveList.size();i++) {
        std::cout << moveList[i] << std::endl;
    }
}

std::string PGN::GetTagPair(std::string symbol) {
    return myTagPairs[symbol];
}
