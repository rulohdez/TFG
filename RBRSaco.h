#ifndef RBRSACO_H
#define RBRSACO_H

#include <vector>
#include <string>
#include "RBRGenerator.h" 

using namespace std;

class RBRSaco {
public:
    static vector<RBRRule> transformToSaco(const vector<RBRRule>& originalRules); // recibe las reglas y lo transforma a saco

private:
    static bool needsFreshVariable(const string& expression); // dice si tiene que convertirse en variable fresca
};

#endif