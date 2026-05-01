#ifndef BLOCK_H
#define BLOCK_H

#include <string>
#include <vector>
#include "Instruction.h"

using namespace std;

class Block {
public:
    enum class Type {
        Entry,              // si es inicial
        Normal,             // si es normal, sin saltos
        ConditionalJump,    // si termina con JUMPI
        UnconditionalJump,  // si termina con JUMP
        Exit,               // si termina el programa (STOP, RETURN, REVERT)
        Unknown             // si no puede saber q tipo de bloque es
    };

    Block(int id, Type type, size_t startPc);

    int getId() const { return id; }
    Type getType() const { return type; }
    size_t getStartPc() const { return pcIni; }

    const std::vector<Instruction>& getInstructions() const { 
        return instructions; 
    }

    const std::vector<int>& getSuccessors() const { 
        return successors; 
    }

    const std::vector<size_t>& getJumpDests() const { 
        return jumpDests; 
    }

    void addInstruction(const Instruction& instr);
    void addSuccessor(int blockId);
    void addJumpDest(size_t dest);

    void setType(Type newType) { 
        type = newType; 
    }

    bool isExitBlock() const;
    std::string getTypeName() const;
    void print() const;

private:
    int id; // id del bloque
    int pcIni; // pc de la primera instruccion
    Type type; //tipo de bloque
    vector<Instruction> instructions; // instrucciones que tiene el bloque
    std::vector<int> successors; // id de los bloques destinos
    std::vector<size_t> jumpDests; // direcciones posibles de salto
};

#endif