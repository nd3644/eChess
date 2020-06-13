#ifndef PGN_H
#define PGN_H

#include <string>
#include <map>
#include <vector>

class PGN {
    public:
        PGN();
        ~PGN();

        void Load(std::string sfilename);

        std::string GetTagPair(std::string symbol);
        std::string GetMove(int i); 
        int GetTotalMoves();
    private:
        std::map<std::string, std::string>myTagPairs;
        std::vector<std::string>moveList;
        int iCurrentMove;
        int iTotalMoves;
};


#endif

