#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <string>
#include <vector>
#include <unordered_map>

using namespace std;

class Instruction {
private:
    string opcodeHex; // numero de la operacion en hexadecimal
    string opcodeName; // nombre de la operacion
    vector<string> arguments; // posibles argumentos
    size_t pc; // posicion en el bytecode

public:
    Instruction(const string& opcodeHex,
        const string& opcodeName,
        const vector<string>& arguments,
        size_t pc);

    const string& getOpcodeHex() const;
    const string& getOpcodeName() const;
    const vector<string>& getArguments() const;
    size_t getPC() const;

    string toString() const; // muestra la instrucción

    // Tablas estáticas
    static const unordered_map<string, string>& getOpcodeNames(); // devuelve el nombre de la instruccion segun su opcodehex
    static const unordered_map<string, size_t>& getOpcodeArgSizes(); // devuelve cuantos bytes siguen a la instruccion
};

#endif
