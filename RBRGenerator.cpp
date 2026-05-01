#include "RBRGenerator.h"
#include <algorithm>
#include <fstream>
#include <unordered_map> 
#include <unordered_set>

using namespace std;

static string hexToDec(const string& hexStr) { // funcion para pasar de hexadecimal a decimal
    string result = "0";
    for (char c : hexStr) {
        int val = (c >= '0' && c <= '9') ? (c - '0') :
            (c >= 'a' && c <= 'f') ? (c - 'a' + 10) :
            (c >= 'A' && c <= 'F') ? (c - 'A' + 10) : 0;
        int carry = val;
        for (int i = result.size() - 1; i >= 0; --i) {
            int curr = (result[i] - '0') * 16 + carry;
            result[i] = (curr % 10) + '0';
            carry = curr / 10;
        }
        while (carry > 0) {
            result.insert(result.begin(), (carry % 10) + '0');
            carry /= 10;
        }
    }
    return result;
}

// recibe la instruccion y el tamaño de la pila
vector<RBRInstruction> RBRGenerator::translateInstruction(const Instruction& instr, int& n, string& lastPushedSlot, const unordered_map<int, unordered_set<string>>& memoryAccesses) {
    vector<RBRInstruction> result;
    string op = instr.getOpcodeName(); // nombre de la instruccion

    if (op.substr(0, 4) == "PUSH") { // mete en la pila
        string arg;
        if (op == "PUSH0") { 
            arg = "0"; // PUSH0 siempre mete un 0
        }
        else {
            string hexArg = instr.getArguments().empty() ? "???" : instr.getArguments()[0]; // metemos los argumentos y si esta vacio "???"
            if (hexArg == "???") {
                arg = "???";
            }
            else {
                arg = hexToDec(hexArg); 
            } 
        }
        lastPushedSlot = arg;
        result.push_back(RBRInstruction("s(" + to_string(n) + ")", arg)); // meto el argumento a la posicion de la pila
        n++; // la pila crece
    }
    else if (op.substr(0, 3) == "DUP") { // copia en la pila 
        int x = stoi(op.substr(3)); // cojo el número del DUP y lo convierto en string (ej: de DUP3 cojo el 3)
        if (n >= x) { // compruebo que hay suficientes elementos en la pila 
            result.push_back(RBRInstruction("s(" + to_string(n) + ")", "s(" + to_string(n - x) + ")")); // meto la instruccion n - x en la cima
        }
        n++; // la pila crece
    }
    else if (op.substr(0, 4) == "SWAP") { // intercambia el top de la pila por la posicion x
        int x = stoi(op.substr(4)); // lugar a intercambiar
        result.push_back(RBRInstruction("s(" + to_string(n) + ")", "s(" + to_string(n - 1) + ")")); // guardo temporalmente el elemento de la cima de la pila en el tope de la pila
        result.push_back(RBRInstruction("s(" + to_string(n - 1) + ")", "s(" + to_string(n - 1 - x) + ")")); // muevo el elemento de la posicion x a la cima
        result.push_back(RBRInstruction("s(" + to_string(n - 1 - x) + ")", "s(" + to_string(n) + ")")); // muevo el elemento de la cima a la posicion x
    }
    else if (op == "ADD" || op == "SUB" || op == "MUL" || op == "DIV") { // operaciones
        string symbol = (op == "ADD") ? "+" : (op == "SUB") ? "-" : (op == "MUL") ? "*" : "/"; // simbolo de la operacion
        string expr = "s(" + to_string(n - 1) + ") " + symbol + " s(" + to_string(n - 2) + ")"; // escribe la operacion
        result.push_back(RBRInstruction("s(" + to_string(n - 2) + ")", expr)); // guarda la expresion en n - 2
        n--; // la pila decrece
    }
    else if (op == "SHR" || op == "SHL" || op == "SAR") { // funciones logicas
        string func = op; 
        transform(func.begin(), func.end(), func.begin(), ::tolower); // pasa a minusculas (ej: shr)
        string expr = func + "(s(" + to_string(n - 1) + "), s(" + to_string(n - 2) + "))"; // hago la expresion
        result.push_back(RBRInstruction("s(" + to_string(n - 2) + ")", expr)); // guardo la expresion en n - 2
        n--; // la pila decrece
    }
    else if (op == "LT" || op == "GT" || op == "EQ") {  // funciones condiciones, coge 2 elementos los compara y si es cierto devuelve 1 sino 0
        string func = (op == "LT") ? "lt" : (op == "GT") ? "gt" : "eq"; // guarda el nombre en minuscula
        string expr = func + "(s(" + to_string(n - 1) + "), s(" + to_string(n - 2) + "))"; // hago la expresion
        result.push_back(RBRInstruction("s(" + to_string(n - 2) + ")", expr)); // guardo la expresion en n - 2
        n--; // la pila decrece
    }
    else if (op == "ISZERO") { // funcion de si el tope de la pila es 0
        result.push_back(RBRInstruction("s(" + to_string(n - 1) + ")", "eq(s(" + to_string(n - 1) + "), 0)")); // guardo en el tope la expresion
    }
    else if (op == "MLOAD") { // lee un dato en memoria y lo mete en la pila, sacando el top
        string memVar = "MEM[s(" + to_string(n - 1) + ")]"; // Texto por defecto (si hay dudas)

        auto it = memoryAccesses.find(instr.getPC());
        // Si la instrucción existe en el diccionario y SOLO tocó 1 dirección constante
        if (it != memoryAccesses.end() && it->second.size() == 1) {
            string addr = *it->second.begin(); // Sacamos esa única dirección
            if (addr != "unknown" && addr != "???") {
                memVar = "l(l" + addr + ")"; // La bautizamos (Ej: l(l64))
            }
        }
        result.push_back(RBRInstruction("s(" + to_string(n - 1) + ")", memVar));
    }
    else if (op == "MSTORE") { // guarda en memoria un valor
        string memVar = "MEM[s(" + to_string(n - 1) + ")]"; // Texto por defecto

        auto it = memoryAccesses.find(instr.getPC());
        if (it != memoryAccesses.end() && it->second.size() == 1) {
            string addr = *it->second.begin();
            if (addr != "unknown" && addr != "???") {
                memVar = "l(l" + addr + ")"; // Ej: l(l64)
            }
        }
        result.push_back(RBRInstruction(memVar, "s(" + to_string(n - 2) + ")"));
        n -= 2; // consume 2 elementos de la pila
    }
    else if (op == "SLOAD") { // lee de storage
        string varName = (lastPushedSlot == "") ? "s(" + to_string(n - 1) + ")" : "field" + lastPushedSlot;
        result.push_back(RBRInstruction("s(" + to_string(n - 1) + ")", "g(" + varName + ")"));
    }
    else if (op == "SSTORE") { // guarda en storage
        string varName = (lastPushedSlot == "") ? "s(" + to_string(n - 1) + ")" : "field" + lastPushedSlot;
        result.push_back(RBRInstruction("g(" + varName + ")", "s(" + to_string(n - 2) + ")"));
        n -= 2;
    }
    else if (op == "CALLDATALOAD") { // lee lo que le envian y sustituye el top por eso
        result.push_back(RBRInstruction("s(" + to_string(n - 1) + ")", "l(calldataload)"));
    }
    else if (op == "CALLDATASIZE" || op == "ADDRESS" || op == "CALLVALUE" || op == "SELFBALANCE") { // variable de la blockchain
        string varName = op;
        transform(varName.begin(), varName.end(), varName.begin(), ::tolower); // lo paso a minuscula
        result.push_back(RBRInstruction("s(" + to_string(n) + ")", "l(" + varName + ")")); // meto la expresion
        n++; // la pila crece
    }
    else if (op == "POP" || op == "JUMP") { // Operaciones que sacan 1 elemento
        n -= 1;
    }
    else if (op == "JUMPI" || op == "RETURN" || op == "REVERT") { // Operaciones que sacan 2 elementos
        n -= 2;
    }
    else if (op.substr(0, 3) == "LOG") { // LOG saca 2 (offset y size) + el número de topics
        int numTopics = stoi(op.substr(3)); // Ej: en LOG1 saca el 1
        n -= (2 + numTopics); // En LOG1 restará 3, en LOG2 restará 4...
    }


    result.push_back(RBRInstruction(op)); // meto el nop

    return result;
}

// recibe el arbol de nodos y la lista de todos los bloques
vector<RBRRule> RBRGenerator::generateRBR(const vector<CreateBlocks::StateNode>& executionTree, const vector<Block>& blocks) {
    vector<RBRRule> allRules; // mete cada regla

    //map<int, int> pcVisitCount;
    unordered_map<int, unordered_set<int>> ancestors; // todos los nodos que hay que pasar hasta llegar hasta el
    unordered_map<int, unordered_set<string>> requiredVariables; // que varaibles globales usa
    unordered_map<int, unordered_set<string>> memoryAccesses; // diccionario de memoria (key: pc counter | value: direcciones de memoria) asi sabre si se usa varias direcciones de memoria en un mismo pc

    bool cambios = true;
    while (cambios) {
        cambios = false;
        for (size_t i = 0; i < executionTree.size(); ++i) { // recorro todos los bloques

            for (int sucesor : executionTree[i].successors) { // miramos los hijos sucesores del bloque i

                if (ancestors[sucesor].insert(i).second) { // intenta meter al bloque i como antecesor, si ya estaba devuelve false
                    cambios = true;
                }

                for (int ancestroDeI : ancestors[i]) { // recorre todos los antecesores
                    if (ancestors[sucesor].insert(ancestroDeI).second) { // mete a sus antecesores
                        cambios = true;
                    }
                }
            }
        }
    }

    for (size_t i = 0; i < executionTree.size(); ++i) { // recorro todos los bloques
        const Block& b = blocks[executionTree[i].blockId]; // obtengo el bloque

        vector<string> tempStack;

        for (const auto& instr : b.getInstructions()) { // recorro las instrucciones del bloque
            string op = instr.getOpcodeName();
            
            if (op.substr(0, 4) == "PUSH") {
                if (op == "PUSH0") tempStack.push_back("0");
                else {
                    string hexArg = instr.getArguments().empty() ? "???" : instr.getArguments()[0];
                    tempStack.push_back(hexArg == "???" ? "???" : hexToDec(hexArg));
                }
            }

            // miro si tiene alguna palabra de blockchain
            if (op == "CALLVALUE" || op == "CALLDATASIZE" || op == "CALLDATALOAD" || op == "SELFBALANCE" || op == "CALLER" || op == "ADDRESS") {

                string varName = op;
                transform(varName.begin(), varName.end(), varName.begin(), ::tolower); // lo paso a minusculas

                varName = "l(" + varName + ")";

                requiredVariables[i].insert(varName); // pongo que este nodo necesita esta variable

                for (int ancestro : ancestors[i]) { // hago que todos sus antecesores tengan esta variable
                    requiredVariables[ancestro].insert(varName);
                }
            }

            if (op == "SLOAD" || op == "SSTORE") {
                string slot = "unknown";
                if (!tempStack.empty()) {
                    slot = tempStack.back(); // Miramos el último PUSH
                }

                string varName = "g(field" + slot + ")"; // Ej: g(field0)

                requiredVariables[i].insert(varName);
                for (int ancestro : ancestors[i]) {
                    requiredVariables[ancestro].insert(varName);
                }
            }

            if (op == "MLOAD" || op == "MSTORE") { // miro si tiene alguna palabra de memoria
                string address = "unknown";

                if (!tempStack.empty()) { // saco la direccion de memoria destino
                    address = tempStack.back();
                }

                memoryAccesses[instr.getPC()].insert(address); // guardamos en el diccionario el PC de la instr y de valor la dirección tocada

                if (address != "unknown" && address != "???") {
                    string varName = "l(l" + address + ")";

                    requiredVariables[i].insert(varName); // lo necesito
                    for (int ancestro : ancestors[i]) {   // los antecesores tambien
                        requiredVariables[ancestro].insert(varName);
                    }
                }
            }
            
        }
    }

    unordered_map<int, string> nodeNames;
    unordered_map<int, int> tempVisitCount;
    for (size_t i = 0; i < executionTree.size(); ++i) {
        int pc = blocks[executionTree[i].blockId].getStartPc();
        int visitIndex = tempVisitCount[pc]++;
        string suffix = (visitIndex > 0) ? "_" + to_string(visitIndex - 1) : "";
        nodeNames[i] = to_string(pc) + suffix;
    }

    int nodeIndex = 0;
    for (const auto& node : executionTree) { // recorre todo el arbol
        const Block& b = blocks[node.blockId]; // coge el bloque de este nodo para poder ver todas sus instrucciones
        RBRRule baseRule; // la regla que añadiremos a allRules

        string pcName = nodeNames[nodeIndex];
        baseRule.name = "block" + pcName; // nombre de la regla (ej: block15_1)

        baseRule.guard = "true"; // se va a ejecutar
       
        int n = node.initialStackSize; // el numero de elementos de la pila antes de ejecutar el bloque

        string inArgs = ""; // argumentos de entrada
        for (int i = n - 1; i >= 0; --i) { // recorro la pila para meterlo en los argumentos
            inArgs += "s(" + to_string(i) + "), "; 
        }
        inArgs += "MEM"; // le metemos la memoria, el storage y la blockchain
        for (const string& var : requiredVariables[nodeIndex]) {
            inArgs += ", " + var; // Añadimos las variables vivas de la blockchain
        }

        baseRule.parameters = { inArgs }; // argumentos de la regla

        string lastPushedSlot = "";
        // Traduce todas las instrucciones del bloque
        for (const auto& instr : b.getInstructions()) { // recorre todas las instrucciones
            vector<RBRInstruction> translated = translateInstruction(instr, n, lastPushedSlot, memoryAccesses);  // cambia el tamaño de n en funcion de la instruccion ademas de coger la instruccion
            baseRule.instructions.insert(baseRule.instructions.end(), translated.begin(), translated.end());  // mete la instruccion a la regla
        }

        // miramos la pila que se nos ha quedado al final de ejecutar el bloque
        string outArgs = ""; 
        for (int i = n - 1; i >= 0; --i) {
            outArgs += "s(" + to_string(i) + "), ";
        }
        outArgs += "MEM";
        for (const string& var : requiredVariables[nodeIndex]) {
            outArgs += ", " + var;
        }

        if (b.getType() == Block::Type::ConditionalJump && node.successors.size() == 2) { // comprueba si es un salto condicional
            string jumpRuleName = "jump" + pcName; // nombre intermedio
            //baseRule.nextCall = "call(" + jumpRuleName + "(" + outArgs + "))"; // ponemos que la regla llame a esta regla intermedia con los argumentos

            int sz = baseRule.instructions.size();
            string condOpcode = "???";
            if (sz >= 5) condOpcode = baseRule.instructions[sz - 4].originalOpcode; // leo la instruccion Ej: ISZERO, LT, EQ

            // calculo el tamaño de la pila antes de que se ejecute esa condición
            int n_before = n;
            if (condOpcode == "ISZERO") n_before = n - 1;
            else if (condOpcode == "LT" || condOpcode == "GT" || condOpcode == "EQ") n_before = n;

            // monto los argumentos limpios 
            string jumpOutArgs = "";
            for (int i = n_before - 1; i >= 0; --i) {
                jumpOutArgs += "s(" + to_string(i) + "), ";
            }
            jumpOutArgs += "MEM";
            for (const string& var : requiredVariables[nodeIndex]) {
                jumpOutArgs += ", " + var;
            }

            // contruyo las guardas lógicas dinamicamente según la operación
            string guardTrue, guardFalse;
            if (condOpcode == "ISZERO") {
                // si la entrada es 0, ISZERO da 1 (True), y JUMPI salta.
                guardTrue = "eq(s(" + to_string(n_before - 1) + "), 0)";
                guardFalse = "neq(s(" + to_string(n_before - 1) + "), 0)";
            }
            else {
                // si es LT, GT, EQ, sacamos la inversa automáticamente
                string func = condOpcode;
                transform(func.begin(), func.end(), func.begin(), ::tolower);
                string invFunc = (func == "eq") ? "neq" : (func == "lt") ? "geq" : (func == "gt") ? "leq" : "???";

                guardTrue = func + "(s(" + to_string(n_before - 1) + "), s(" + to_string(n_before - 2) + "))";
                guardFalse = invFunc + "(s(" + to_string(n_before - 1) + "), s(" + to_string(n_before - 2) + "))";
            }

            if (sz >= 5) {
                string pushOpcode = baseRule.instructions[sz - 2].originalOpcode;
                string jumpiOpcode = baseRule.instructions[sz - 1].originalOpcode;
                baseRule.instructions.erase(baseRule.instructions.end() - 5, baseRule.instructions.end());

                // inseto el call usando los argumentos limpios que acabamos de calcular
                baseRule.instructions.push_back(RBRInstruction("call(" + jumpRuleName + "(" + jumpOutArgs + "))", true));
                baseRule.instructions.push_back(RBRInstruction(condOpcode));
                baseRule.instructions.push_back(RBRInstruction(pushOpcode));
                baseRule.instructions.push_back(RBRInstruction(jumpiOpcode));
            }
            baseRule.nextCall = "HIDDEN";

            string succ0_PC = nodeNames[node.successors[0]];
            string succ1_PC = nodeNames[node.successors[1]];

            RBRRule jumpFalse;
            jumpFalse.name = jumpRuleName;
            jumpFalse.parameters = { jumpOutArgs };
            jumpFalse.guard = guardFalse;
            jumpFalse.nextCall = "call(block" + succ1_PC + "(" + jumpOutArgs + "))";
            allRules.push_back(jumpFalse);

            RBRRule jumpTrue;
            jumpTrue.name = jumpRuleName;
            jumpTrue.parameters = { jumpOutArgs };
            jumpTrue.guard = guardTrue;
            jumpTrue.nextCall = "call(block" + succ0_PC + "(" + jumpOutArgs + "))";
            allRules.push_back(jumpTrue);

            allRules.push_back(baseRule);
        }
        else if (node.successors.size() == 1) { // si solo tiene 1 argumento
            string succ0_PC = nodeNames[node.successors[0]];
            baseRule.nextCall = "call(block" + succ0_PC + "(" + outArgs + "))"; // lo meto en la llamada
            allRules.push_back(baseRule); // guardo la regla
        }
        else { // no hay mas bloque
            baseRule.nextCall = ""; // no meto nada en llamada
            allRules.push_back(baseRule);
        }

        nodeIndex++;
    }

    return allRules;
}

void RBRGenerator::printRBR(const vector<RBRRule>& rules, const string& filename) {
    ofstream outFile(filename);

    cout << "\n=== Representacion Basada en Reglas (RBR) -> " << filename << " ===\n" << endl;

    for (const auto& rule : rules) { // recorro todas las reglas
        cout << rule.name << "("; 
        outFile << rule.name << "(";
        for (size_t i = 0; i < rule.parameters.size(); ++i) { // recorro todos los parametros (deberia de haber solo 1 por ahora)
            cout << rule.parameters[i];
            outFile << rule.parameters[i];
            if (i < rule.parameters.size() - 1) {
                cout << ", ";
                outFile << ", ";
            }
        }
        cout << ") =>" << endl;
        outFile << ") =>" << endl;

        if (rule.guard != "true") {
            cout << "    " << rule.guard << endl;
            outFile << "    " << rule.guard << endl;
        }

        for (const auto& instr : rule.instructions) { // imprime las instrucciones
            cout << "    ";
            outFile << "    ";
            if (instr.type == RBRInstrType::Assignment) {
                cout << instr.assignee << " = " << instr.expression; // Ej: s_2 = s_1 + s_0
                outFile << instr.assignee << " = " << instr.expression;
            }
            else if (instr.type == RBRInstrType::Nop) {
                cout << "nop(" << instr.originalOpcode << ")"; // Ej: nop(ADD)
                outFile << "nop(" << instr.originalOpcode << ")";
            }
            else if (instr.type == RBRInstrType::Call) {
                cout << instr.callText; // ej: call(jump0(...))
                outFile << instr.callText;
            }
            cout << endl;
            outFile << endl;
        }

        if (!rule.nextCall.empty()) { // muestra la siguiente llamada
            if (rule.nextCall != "HIDDEN") { // ignoramos si está oculto
                cout << "    " << rule.nextCall << endl;
                outFile << "    " << rule.nextCall << endl;
            }
        }
        else {
            cout << "    // FIN DE EJECUCION (Exit / Revert / Stop)" << endl;
            outFile << "    // FIN DE EJECUCION (Exit / Revert / Stop)" << endl;
        }

        cout << endl; 
        outFile << endl;
    }

    cout << "=============================================\n" << endl;
}