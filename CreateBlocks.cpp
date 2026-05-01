#include "CreateBlocks.h"

using namespace std;

struct SimulationResult {
    vector<string> finalStack;
    long long jumpTarget = -1; // -1 si no hay salto o es desconocido
};

static int getStackDelta(const string& opcode) { // devuelve la cantidad en la que crece o decrece la pila
    if (opcode.substr(0, 4) == "PUSH") return 1;
    if (opcode.substr(0, 3) == "DUP") return 1;
    if (opcode.substr(0, 4) == "SWAP") return 0; 
    if (opcode == "POP") return -1;

    if (opcode == "LOG0") return -2;
    if (opcode == "LOG1") return -3;
    if (opcode == "LOG2") return -4;
    if (opcode == "LOG3") return -5;
    if (opcode == "LOG4") return -6;

    if (opcode == "ADD" || opcode == "MUL" || opcode == "SUB" || opcode == "DIV" ||
        opcode == "SDIV" || opcode == "MOD" || opcode == "SMOD" || opcode == "EXP" ||
        opcode == "SIGNEXTEND" || opcode == "LT" || opcode == "GT" || opcode == "SLT" ||
        opcode == "SGT" || opcode == "EQ" || opcode == "AND" || opcode == "OR" ||
        opcode == "XOR" || opcode == "BYTE" || opcode == "SHL" || opcode == "SHR" ||
        opcode == "SAR") return -1;

    if (opcode == "CALL" || opcode == "CALLCODE") return -6;
    if (opcode == "DELEGATECALL" || opcode == "STATICCALL") return -5;
    if (opcode == "CREATE") return -2;
    if (opcode == "CREATE2") return -3;

    if (opcode == "ADDMOD" || opcode == "MULMOD") return -2;

    if (opcode == "ADDRESS" || opcode == "ORIGIN" || opcode == "CALLER" ||
        opcode == "CALLVALUE" || opcode == "CALLDATASIZE" ||
        opcode == "CODESIZE" || opcode == "GASPRICE" || opcode == "RETURNDATASIZE" ||
        opcode == "PC" || opcode == "MSIZE" || opcode == "GAS" || opcode == "CHAINID" ||
        opcode == "BASEFEE" || opcode == "SELFBALANCE") return 1;

    if (opcode == "CALLDATACOPY" || opcode == "CODECOPY" || opcode == "RETURNDATACOPY" || opcode == "MCOPY") return -3;
    if (opcode == "EXTCODECOPY") return -4;

    if (opcode == "ISZERO" || opcode == "NOT" || opcode == "BALANCE" ||
        opcode == "CALLDATALOAD" || opcode == "EXTCODESIZE" ||
        opcode == "EXTCODEHASH" || opcode == "BLOCKHASH") return 0;

    if (opcode == "SHA3") return -1;

    if (opcode == "BALANCE") return 0;

    if (opcode == "MLOAD" || opcode == "SLOAD") return 0;

    if (opcode == "MSTORE" || opcode == "MSTORE8" || opcode == "SSTORE") return -2;

    return 0;
}

static SimulationResult runBlockSimulation(const Block& block, vector<string> stack) { // simula la ejecucion del bloque
    SimulationResult result;

    for (const auto& instr : block.getInstructions()) { // recorre cada instruccion del bloque
        string op = instr.getOpcodeName();

        if (op.substr(0, 4) == "PUSH") { // mete datos
            if (!instr.getArguments().empty())  // comprueba que tiene argumento y lo mete
                stack.push_back(instr.getArguments()[0]);
            else 
                stack.push_back("???");
        }
        else if (op.substr(0, 3) == "DUP") { // copiamos el dato n de la pila empezando por el final
            int n = stoi(op.substr(3));
            if (stack.size() >= n) 
                stack.push_back(stack[stack.size() - n]); 
            else 
                stack.push_back("???"); 
        }
        else if (op.substr(0, 4) == "SWAP") { // intercambia los 2 ultimos elementos
            int n = stoi(op.substr(4));
            if (stack.size() > n) 
                std::iter_swap(stack.end() - 1, stack.end() - 1 - n);
        }
        else if (op == "JUMP" || op == "JUMPI") { // salta al ultimo valor que tiene la pila
            if (!stack.empty()) {
                string targetHex = stack.back(); // coge el ultimo valor de la pila
                if (targetHex != "???") { // compruebo que sea conocido
                    try {
                        result.jumpTarget = stoul(targetHex, nullptr, 16); // guardo el destino, convierte el texto a numero hexadecimal
                    }
                    catch (const std::exception& e) {
                        result.jumpTarget = -1;
                        cerr << "Fallo la conversion a numero hexadecimal" << endl;
                    }
                }
            }
            stack.pop_back(); // lo elimino de la pila
            if (op == "JUMPI" && !stack.empty()) 
                stack.pop_back(); // Consumir condición
        }
        else {
            int delta = getStackDelta(op); // devuelve como deberia de quedar la pila
            if (delta < 0) {
                int toPop = -delta;
                while (toPop > 0 && !stack.empty()) {
                    stack.pop_back();
                    toPop--;
                }
            }
            else if (delta > 0) {
                for (int k = 0; k < delta; ++k) 
                    stack.push_back("???");
            }
        }
    }

    result.finalStack = stack;
    return result;
}

static string getStateKey(int blockId, const vector<string>& stack) { // convierte toda la pila de un bloque es un string
    stringstream ss;
    ss << blockId << ":";
    for (const auto& s : stack) 
        ss << s << ",";
    return ss.str();
}

bool CreateBlocks::isBlockStart(const vector<Instruction>& instr, size_t i) { // determina si se está empezando un bloque
    if (i == 0)
        return true;
    else if (instr[i].getOpcodeName() == "JUMPDEST") // si es jumpdest empieza
        return true;
    else if (i > 0 && isBlockTerminator(instr[i - 1])) // si la instruccion anterior termina un bloque, empieza
        return true;
    return false;
}

Block::Type CreateBlocks::determineBlockType(const Instruction& instr) { // determina el tipo de bloque
    string opcode = instr.getOpcodeName();

    if (opcode == "STOP" || opcode == "RETURN" || opcode == "REVERT" || opcode == "SELFDESTRUCT") {
        return Block::Type::Exit;
    } else if (opcode == "JUMP") {
        return Block::Type::UnconditionalJump;
    } else if (opcode == "JUMPI") {
        return Block::Type::ConditionalJump;
    } else if (instr.getPC() == 0) {
        return Block::Type::Entry;
    }

    return Block::Type::Normal;
}

bool CreateBlocks::isBlockTerminator(const Instruction& instr) { // define que instrucciones terminan un bloque
    string op = instr.getOpcodeName();
    return (op == "JUMP" || op == "JUMPI" || op == "STOP" || op == "RETURN" || op == "REVERT" || op == "SELFDESTRUCT");
}

vector<Block> CreateBlocks::buildBasicBlocks(const vector<Instruction>& instructions) {
    vector<Block> blocks;
    if (instructions.empty()) 
        return blocks;

    int currentBlockId = 0; 
    Block currentBlock(currentBlockId, Block::Type::Entry, instructions.front().getPC());

    for (size_t i = 0; i < instructions.size(); ++i) { // recorremos todas las instrucciones
        const Instruction& instr = instructions[i]; // saco la instruccion
        const string& opcode = instr.getOpcodeName(); // saco el nombre de la instruccion

        bool startsNewBlock = isBlockStart(instructions, i); // comprueba si deberia ser el inicio de un bloque

        if (startsNewBlock && !currentBlock.getInstructions().empty()) { // reviso si es nuevo bloque y si tiene instrucciones el bloque actual

            const Instruction& lastInstr = currentBlock.getInstructions().back(); // miro cual era la ultima instruccion del anterior
            currentBlock.setType(determineBlockType(lastInstr)); // cambio el tipo de bloque
            blocks.push_back(currentBlock); // guardo el bloque

            currentBlockId++; // aumento el numero de bloques
            currentBlock = Block(currentBlockId, Block::Type::Normal, instr.getPC()); // creo un bloque nuevo con la instr actual
        }

        currentBlock.addInstruction(instr); // añado la instruccion al bloque
    }

    if (!currentBlock.getInstructions().empty()) { // guardo el ultimo bloque si tiene instrucciones
        const Instruction& lastInstr = currentBlock.getInstructions().back(); // cojo la ultima instruccion del bloque
        currentBlock.setType(determineBlockType(lastInstr)); // determino el tipo de bloque
        blocks.push_back(currentBlock); // cambio el tipo del ultimo bloque
    }

    return blocks;
}

void CreateBlocks::linkBlocks(vector<Block>& blocks) { // conecta los bloques
    executionTree.clear();
    blockVisitCount.clear();

    unordered_map<size_t, int> pcToBlockId; // mapa que dice que bloque es cierto indice
    for (const auto& b : blocks) // recorre todos los bloques
        pcToBlockId[b.getStartPc()] = b.getId(); // guarda en el mapa cada id del bloque segun su pc inicial

    struct WorkItem {
        int parentStateId;
        int blockId;
        vector<string> stack;
    };
    queue<WorkItem> worklist; // simulamos la pila con el id del bloque y la pila del bloque

    set<string> visitedStates; // set para mirar si un bloque ya ha sido visitado con la misma pila, asi evito bucles

    worklist.push({ -1, {} }); // inicializo con el bloque 0 sin pila
    visitedStates.insert(getStateKey(0, {})); // iniciacizo con el bloque y la pila vacia

    while (!worklist.empty()) { // recorro todos los bloques con pilas
        auto current = worklist.front(); // saco la tarea
        worklist.pop(); // la elimino

        int parentStateId = current.parentStateId;
        int blockId = current.blockId;
        vector<string> inputStack = current.stack; // cojo toda la pila

        Block& block = blocks[blockId]; // cojo la referencia del bloque para poder modificarlos

        int currentStateId = executionTree.size();

        string nodeName = "Block_" + to_string(blockId);
        int visitNum = blockVisitCount[blockId]++;
        if (visitNum > 0) {
            nodeName += "_" + to_string(visitNum - 1);
        }

        executionTree.push_back(StateNode(currentStateId, blockId, nodeName, inputStack.size()));

        if (parentStateId != -1) {
            executionTree[parentStateId].successors.push_back(currentStateId);
        }

        SimulationResult res = runBlockSimulation(block, inputStack); // simula el bloque y la pila

        Block::Type type = block.getType(); // coje el tipo de bloque que es
        int nextId = blockId + 1; // coje el que sería el siguiente bloque


        // El bloque es un (Normal, Entry, ConditionalJump cae al siguiente)
        if (type == Block::Type::Normal || type == Block::Type::Entry || type == Block::Type::ConditionalJump) {
            if (nextId < blocks.size()) { // compruebo que sea un bloque existente
                bool exists = false; // evito duplicados
                for (int s : block.getSuccessors()) // recorro sus sucesores para ver si alguno es el siguiente bloque
                    if (s == nextId) // si lo es marco como que ya existe
                        exists = true;

                if (!exists) // si no existe añado el siguiente bloque como sucesor del actual
                    block.addSuccessor(nextId); 

                string key = getStateKey(nextId, res.finalStack); // añado el siguiente bloque a la cola
                if (visitedStates.find(key) == visitedStates.end()) {
                    visitedStates.insert(key);
                    worklist.push({ currentStateId, nextId, res.finalStack });
                }
            }
        }

        // saltos (Jump, Jumpi)
        if (type == Block::Type::ConditionalJump || type == Block::Type::UnconditionalJump) {

            long long destPc = res.jumpTarget; // cogemos el destino que calculamos en runBlockSimulator

            if (destPc != -1 && pcToBlockId.count(destPc)) { // compruebo que el destino existe
                int targetBlockId = pcToBlockId[destPc]; // pillo que bloque es

                bool exists = false; // compruebo si ya la rama existia
                for (int s : block.getSuccessors()) 
                    if (s == targetBlockId) 
                        exists = true;

                if (!exists) { // si no existe lo añado como sucesor de este bloque
                    block.addSuccessor(targetBlockId);
                    block.addJumpDest(destPc);
                }

                string key = getStateKey(targetBlockId, res.finalStack); // añadimos el destino con la pila 

                if (visitedStates.find(key) == visitedStates.end()) {
                    visitedStates.insert(key);
                    worklist.push({ currentStateId, targetBlockId, res.finalStack });
                }
            }
        }
    }
    deadBlocks.clear();
    for (const auto& b : blocks) {
        if (blockVisitCount.find(b.getId()) == blockVisitCount.end()) {
            deadBlocks.push_back(b.getId()); 
        }
    }
}

void CreateBlocks::printGraph(const vector<Block>& blocks) {
    if (blocks.empty()) return;

    cout << "\n=== Grafo de Control de Flujo (CFG) ===\n" << endl;

    vector<bool> visited(blocks.size(), false);
    unordered_map<int, int> visitCount;

    printGraphRecursive(blocks, 0, visited, "", true, visitCount);

    cout << "\n=======================================\n" << endl;
}


void CreateBlocks::printGraphRecursive(const vector<Block>& blocks, int currentId, vector<bool>& visited, string prefix, bool isLast, unordered_map<int, int>& visitCount) {
    if (currentId < 0 || currentId >= blocks.size()) return;

    const Block& b = blocks[currentId];

    string nodeName = "Block_" + to_string(b.getId());
    int currentVisit = visitCount[currentId]++; // Incrementa y guarda el valor anterior
    if (currentVisit > 0) {
        nodeName += "_" + to_string(currentVisit - 1);
    }

    cout << prefix;
    if (isLast) { cout << "\\-- "; prefix += "    "; }
    else { cout << "|-- "; prefix += "|   "; }

    cout << nodeName << " [" << b.getTypeName() << "]";

    if (visited[currentId]) {
        cout << " ( ... CICLO detectado, deteniendo rama en " << nodeName << " )" << endl;
        return;
    }
    visited[currentId] = true;
    cout << endl;

    const vector<int>& successors = b.getSuccessors();
    for (size_t i = 0; i < successors.size(); ++i) {
        bool lastChild = (i == successors.size() - 1);
        vector<bool> branchVisited = visited;

        printGraphRecursive(blocks, successors[i], branchVisited, prefix, lastChild, visitCount);
    }
}


void CreateBlocks::generateDotFile(const vector<Block>& blocks, const string& filename) {
    ofstream outFile(filename); // crea el archivo

    if (!outFile.is_open()) { 
        cerr << "Error: No se pudo crear el archivo " << filename << endl;
        return;
    }

    outFile << "digraph Grafo {\n";
    outFile << "    node [shape=box, style=filled, fontname=\"Helvetica\"];\n"; // Estilo por defecto

    for (const auto& b : blocks) { // recorro todos los bloques
        string attributes = "fillcolor=\"#ffffff\""; // Blanco por defecto

        if (b.getType() == Block::Type::Entry) // si es el bloque de entrada
            attributes = "fillcolor=\"#ccffcc\""; // Verde claro para entrada
        else if (b.getType() == Block::Type::Exit) // si es el bloque de salida
            attributes = "fillcolor=\"#ffcccc\""; // Rojo claro para salida (Stop/Revert)
        else if (b.getType() == Block::Type::ConditionalJump) // si es un salto condicional
            attributes = "shape=diamond, fillcolor=\"#ffffcc\""; // Rombo amarillo para decisiones

        // muestra el bloque con el formato: Block_ID, Tipo y PC de inicio
        outFile << "    block_" << b.getId() << " [label=\"Block " << b.getId()
            << "\\n(" << b.getTypeName() << ")\\nPC: " << b.getStartPc() << "\""
            << ", " << attributes << "];\n";

        
        const vector<int>& successors = b.getSuccessors();
        for (int succId : successors) { // escribimos todas las aristas
            outFile << "    block_" << b.getId() << " -> block_" << succId << ";\n";
        }
    }

    outFile << "}\n";
    outFile.close();

    cout << "Generado correctamente el archivo: " << filename << endl;
    cout << "Puedes visualizarlo en: https://dreampuf.github.io/GraphvizOnline/" << endl; // donde se puede ver
}

void CreateBlocks::generateExecutionTreeDotFile(const vector<Block>& blocks, const string& filename) {
    ofstream outFile(filename);
    if (!outFile.is_open()) return;

    outFile << "digraph ExecutionTree {\n";
    outFile << "    node [shape=box, style=filled, fontname=\"Helvetica\"];\n";

    for (const auto& node : executionTree) {
        // Encontramos el bloque real
        const Block& b = blocks[node.blockId];
        string attributes = "fillcolor=\"#ffffff\""; // Blanco
        if (b.getType() == Block::Type::Entry) attributes = "fillcolor=\"#ccffcc\""; // Verde
        else if (b.getType() == Block::Type::Exit) attributes = "fillcolor=\"#ffcccc\""; // Rojo
        else if (b.getType() == Block::Type::ConditionalJump) attributes = "shape=diamond, fillcolor=\"#ffffcc\""; // Rombo

        outFile << "    state_" << node.stateId << " [label=\"" << node.name
            << "\\n(" << b.getTypeName() << ")\\nPC: " << b.getStartPc() << "\""
            << ", " << attributes << "];\n";

        for (int succId : node.successors) {
            outFile << "    state_" << node.stateId << " -> state_" << succId << ";\n";
        }
    }
    outFile << "}\n";
    outFile.close();

    cout << "Generado correctamente el archivo: " << filename << endl;
    cout << "Puedes visualizarlo en: https://dreampuf.github.io/GraphvizOnline/" << endl; // donde se puede ver

    if (deadBlocks.empty()) {
        cout << "Todos los bloques son ejecutables." << endl;
    }
    else {
        cout << "Los bloques muertos son los siguientes: " << endl;
        for (int id : deadBlocks) {
            const Block& b = blocks[id];
            cout << "  - Block_" << id << " [" << b.getTypeName() << "] (PC: " << b.getStartPc() << ")" << endl;
        }
    }
}
