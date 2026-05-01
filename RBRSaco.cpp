#include "RBRSaco.h"

using namespace std;

bool RBRSaco::needsFreshVariable(const string& expression) {
    vector<string> forbidden = { // lista de operaciones que se tienen que cambiar por una variable fresca
        "and", "or", "xor", "not", "shr", "shl", "sar", "exp",
        "byte", "signextend", "keccak256", "extcodehash",
        "eq", "lt", "gt", "iszero"
    };

    for (const string& op : forbidden) {
        if (expression.find(op + "(") == 0) { // comprobamos si la expresión empieza exactamente por alguna expresion + "("
            return true; 
        }
    }

    return false;
}

vector<RBRRule> RBRSaco::transformToSaco(const vector<RBRRule>& originalRules) {
    vector<RBRRule> sacoRules = originalRules; // hago una copia para modificarla

    for (auto& rule : sacoRules) { // recorro todas las reglas
        int freshCounter = 0; // el contador se reinicia a 0 al entrar en cada bloque/regla

        for (auto& instr : rule.instructions) { // recorremos sus instrucciones

            // me fijo en las asignaciones 
            if (instr.type == RBRInstrType::Assignment) {

                if (needsFreshVariable(instr.expression)) { // compruebo si hay que cambiarla

                    instr.expression = "f_" + to_string(freshCounter); // la reemplazo

                    freshCounter++; // aumento el contador
                }
            }
        }
    }

    return sacoRules;
}