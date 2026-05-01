#include <iostream>
#include <chrono>
#include "Compiler.h"
#include "Parser.h"
#include "Instruction.h"
#include "CreateBlocks.h"
#include "RBRGenerator.h"
#include "RBRSaco.h"

using namespace std;

int main(int argc, char* argv[]) {
    try {
        if (argc < 2) { // compruebo el numero de argumentos 
            throw invalid_argument("Te falta el argumento que debe ser un .sol");
        }

        string solFile = argv[1];

        cout << "Archivo recibido: " << solFile << endl;
        if (solFile.size() < 4 || solFile.substr(solFile.size() - 4) != ".sol") { // compruebo que el archivo es .sol
            throw invalid_argument("El argumento no es un archivo .sol");
        }

        cout << "Se va a llamar al compilador" << endl;

        auto start_comp = std::chrono::high_resolution_clock::now();
        Compiler compiler;
        string bin = compiler.compile(solFile);
        auto end_comp = std::chrono::high_resolution_clock::now();
        chrono::duration<double> time_comp = end_comp - start_comp;

        cout << "El compilador a devuelto: " << bin << endl;
        
        auto start_cfg = std::chrono::high_resolution_clock::now();
        Parser parser;
        vector<Instruction> instructions = parser.parseBytecode(bin);

        for (const auto& instr : instructions) {
            cout << instr.toString() << endl;
        }

        cout << endl << endl;

        CreateBlocks createBlocks;
        vector<Block> blocks = CreateBlocks::buildBasicBlocks(instructions);

        for (Block b : blocks) {
            b.print();
        }

        createBlocks.linkBlocks(blocks);

        createBlocks.generateExecutionTreeDotFile(blocks, "arbol.dot");

        auto end_cfg = std::chrono::high_resolution_clock::now();
        chrono::duration<double> time_cfg = end_cfg - start_cfg;

        auto start_rbr = std::chrono::high_resolution_clock::now();
        vector<RBRRule> rules = RBRGenerator::generateRBR(createBlocks.executionTree, blocks);
        auto end_rbr = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> time_rbr = end_rbr - start_rbr;

        RBRGenerator::printRBR(rules, "representacion_rbr.txt");

        auto start_saco = std::chrono::high_resolution_clock::now();
        vector<RBRRule> reglasSaco = RBRSaco::transformToSaco(rules);
        auto end_saco = std::chrono::high_resolution_clock::now(); // PARA RELOJ
        chrono::duration<double> time_saco = end_saco - start_saco;

        RBRGenerator::printRBR(reglasSaco, "representacion_rbr_saco.txt");

        cout << "BLOCKS for " << solFile << ": " << createBlocks.executionTree.size() << endl;

        cout << "Compilation time: " << time_comp.count() << "s" << endl;
        cout << "Build CFG: " << time_cfg.count() << "s" << endl;
        cout << "Build RBR: " << time_rbr.count() << "s" << endl;
        cout << "SACO RBR: " << time_saco.count() << "s" << endl;
    }
    catch (const invalid_argument& e) {
        cerr << "Error de argumento: " << e.what() << endl;
        return 1;
    }
    catch (const runtime_error& e) {
        cerr << "Error en ejecucion: " << e.what() << endl;
        return 2;
    }
    

    return 0;
}
