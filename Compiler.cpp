#include "Compiler.h"
#include <stdexcept>
#include <array>
#include <memory>
#include <cstdio> 
#include <iostream>

using namespace std;

string Compiler::compile(const string& filename) {

    string command = "solc --bin-runtime " + filename; // comando que devuelve el runtime en hexadecimal

    array<char, 128> buffer; 
    string result;

    cout << "Se va a llamar al comando: " << command << endl;

    FILE* pipe = _popen(command.c_str(), "r"); // abro una pipe para leer la salida del proceso
    if (!pipe) {
        throw runtime_error("No se pudo ejecutar solc-windows.exe");
    }

    cout << "Se va a intentar leer toda la salida del compilador" << endl;

    try {
        while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) { //leo poco a poco el archivo
            result += buffer.data();
        }
    }
    catch (...) {
        _pclose(pipe);
        throw;
    }

    _pclose(pipe);

    if (result.empty()) {
        throw runtime_error("El compilador no devolvio ninguna salida");
    }

    cout << "Se a conseguido leer correctamente" << endl;

    return result;
}
