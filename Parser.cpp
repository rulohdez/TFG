#include "Parser.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iostream>

using namespace std;

vector<Instruction> Parser::parseBytecode(const string& compilerOutput) {
    
    string hexBytecode = extractHexBytecode(compilerOutput); // se queda con la hexadecimal

    cout << "Se ha limpiado el compilado y ahora es: " << hexBytecode << endl << endl;

    return decodeInstructions(hexBytecode); // lo separa en instrucciones
}

string Parser::extractHexBytecode(const string& compilerOutput) { 
    stringstream ss(compilerOutput);
    string line;
    string hex;
    int lineCount = 0;

    while (getline(ss, line)) {
        lineCount++;

        if (lineCount <= 3) continue; // salto las 2 primeras lineas

        line.erase(remove_if(line.begin(), line.end(), ::isspace), line.end()); // elimino espacios y saltos de linea

        hex += line;
    }

    return hex;
}

vector<Instruction> Parser::decodeInstructions(const string& hex) {
    vector<Instruction> instructions;
    size_t pc = 0;

    while (pc < hex.size()) {
        string opcodeHex = hex.substr(pc, 2); // toma el opcode actual 
        int pcIni = pc / 2;

        //cout << "Se va a leer el opcode en hexadecimal: " << opcodeHex << endl;

        auto& opcodeNames = Instruction::getOpcodeNames(); // obtengo el nombre de la instruccion

        string opcodeName = "";
        if (opcodeNames.find(opcodeHex) == opcodeNames.end()) { // compruebo que el opcode existe
            opcodeName = "unknown";
        } else
            opcodeName = opcodeNames.at(opcodeHex);

        //cout << "Ese opcode es la instruccion " << opcodeName << endl;

        pc += 2; // avanzo el byte
        
        vector<string> args;
        auto& argSizes = Instruction::getOpcodeArgSizes();
        if (argSizes.find(opcodeHex) != argSizes.end()) { // compruebo si la instruccion tiene argumentos
            size_t argSize = argSizes.at(opcodeHex); // compruebo cuantos bytes ocupa
            if (pc + argSize * 2 <= hex.size()) {
                string arg = hex.substr(pc, argSize * 2); // mete en arg los n bytes en hex
                args.push_back(arg);
                pc += argSize * 2; // avanzo el numero de argumentos
                //cout << "Se ha tomado de argumento: " << arg << endl;
            }
        }

        //cout << endl;

        Instruction instr(opcodeHex, opcodeName, args, pcIni);
        instructions.push_back(instr);
    }

    return instructions;
}
