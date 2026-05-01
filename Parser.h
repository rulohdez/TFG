#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include "Instruction.h"

using namespace std;

class Parser {
public:

    static vector<Instruction> parseBytecode(const string& compilerOutput);

private:
    static string extractHexBytecode(const string& compilerOutput); // quita el encabezado

    static vector<Instruction> decodeInstructions(const string& hexBytecode); // divide el string en instrucciones

};

#endif
