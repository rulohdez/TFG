#include "Block.h"
#include <iostream>

using namespace std;

Block::Block(int id, Type type, size_t startPc) : id(id), pcIni(startPc), type(type) {}

void Block::addInstruction(const Instruction& instr) {
    instructions.push_back(instr);
}

void Block::addSuccessor(int blockId) {
    successors.push_back(blockId);
}

void Block::addJumpDest(size_t dest) {
    jumpDests.push_back(dest);
}

bool Block::isExitBlock() const {
    return type == Type::Exit;
}

string Block::getTypeName() const {
    switch (type) {
        case Type::Entry: return "Entry";
        case Type::Normal: return "Normal";
        case Type::ConditionalJump: return "ConditionalJump";
        case Type::UnconditionalJump: return "UnconditionalJump";
        case Type::Exit: return "Exit";
        case Type::Unknown: return "Unknown";
        default: return "Undefined";
    }
}

void Block::print() const {
    cout << "Block_" << id << " (" << getTypeName() << ")" << endl;
    cout << "  Start PC: " << pcIni << endl;

    if (!jumpDests.empty()) {
        cout << "  JumpDests: ";
        for (size_t i = 0; i < jumpDests.size(); ++i) {
            cout << jumpDests[i];
            if (i != jumpDests.size() - 1) cout << ", ";
        }
        cout << "\n";
    }

    if (!successors.empty()) {
        cout << "  Successors: ";
        for (size_t i = 0; i < successors.size(); ++i) {
            cout << successors[i];
            if (i != successors.size() - 1) cout << ", ";
        }
        cout << "\n";
    }

    cout << "  Instructions:" << endl;
    for (const auto& instr : instructions) {
        cout << "    " << instr.getPC() << ": " << instr.getOpcodeName();
        if (!instr.getArguments().empty()) {
            vector<string> arguments = instr.getArguments();
            for (const auto& a : arguments) {
                cout << " " << a;
            }
        }
        cout << endl;
    }

    cout << endl;
}
