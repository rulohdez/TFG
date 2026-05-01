#ifndef CREATEBLOCKS_H
#define CREATEBLOCKS_H

#include <vector>
#include <unordered_map>
#include "Instruction.h"
#include "Block.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <queue>
#include <set>
#include <sstream>
#include <unordered_map>

using namespace std;

class CreateBlocks {
public:
    struct StateNode {
        int stateId;                 // id unico del nodo con el id del bloque
        int blockId;                 // id del bloque
        std::string name;            // nombre visual
        std::vector<int> successors; // sucesores

        int initialStackSize;

        StateNode(int sId, int bId, std::string n, int stackSize) : stateId(sId), blockId(bId), name(n), initialStackSize(stackSize) {}
    };
    vector<StateNode> executionTree;

    static vector<Block> buildBasicBlocks(const vector<Instruction>& instructions);
    void linkBlocks(std::vector<Block>& blocks); // enlaza los bloques

    static void printGraph(const std::vector<Block>& blocks);
    static void generateDotFile(const vector<Block>& blocks, const string& filename);

    void generateExecutionTreeDotFile(const vector<Block>& blocks, const std::string& filename);

private:
    static bool isBlockStart(const vector<Instruction>& instr, size_t i); // determina si una instruccion marca el inicio de un nuevo bloque

    static bool isBlockTerminator(const Instruction& instr);

    static Block::Type determineBlockType(const Instruction& instr); // determina el tipo de bloque

    static void printGraphRecursive(const vector<Block>& blocks, int currentId, vector<bool>& visited, string prefix, bool isLast, unordered_map<int, int>& visitCount);

    unordered_map<int, int> blockVisitCount;

    vector<int> deadBlocks;
};

#endif