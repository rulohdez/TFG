#ifndef RBRGENERATOR_H
#define RBRGENERATOR_H

#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include "Instruction.h"   
#include "CreateBlocks.h"

using namespace std;

enum class RBRInstrType { // tipos de instrucciones
    Assignment, // operacion matemarica (ej: s_2 = s_1 + s_0)
    Nop, // no operacion sino la instruccion como tal
    Call // a donde llama
};

// LA INSTRUCCIÓN RBR
struct RBRInstruction { // estructura de cada instruccion
    RBRInstrType type; // el tipo de la instruccion

    // si es una asignación (ej: s_5 = s_4 + 10)
    string assignee; // la variable que recibe el dato (ej: s_5)
    string expression; // la operación matematica (ej: "s_4 + 10")

    // si es un NOP guardamos la instruccion (ej: nop(ADD))
    string originalOpcode;

    string callText; // texto de la llamada

    RBRInstruction(string assign, string expr)
        : type(RBRInstrType::Assignment), assignee(assign), expression(expr) {}

    RBRInstruction(string opcode)
        : type(RBRInstrType::Nop), originalOpcode(opcode) {}

    RBRInstruction(string callT, bool isCall)
        : type(RBRInstrType::Call), callText(callT) {}
};

struct RBRRule { // la regla
    string name;                    // nombre de la regla (ej: "block_5_0" o "jump_5_0")
    vector<string> parameters;      // parametros de la regla (ej: ["s_0", "s_1", "MEM", "STO", "BLC"])
    string guard;                   // condicion para entrar (ej: "true", "s_3 != 0", "s_3 == 0")
    vector<RBRInstruction> instructions; // instrucciones de operaciones y nops
    string nextCall;                // bloque al que salta (ej: "call(block_6_0(s_0, s_1, MEM, STO, BLC))")
};

class RBRGenerator {
public:
    static vector<RBRRule> generateRBR(const vector<CreateBlocks::StateNode>& executionTree, const vector<Block>& blocks);

    static void printRBR(const vector<RBRRule>& rules, const string& filename);

private:
    static vector<RBRInstruction> translateInstruction(const Instruction& instr, int& n, string& lastPushedSlot, const unordered_map<int, unordered_set<string>>& memoryAccesses);
    
};

#endif