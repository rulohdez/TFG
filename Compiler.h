#ifndef COMPILER_H
#define COMPILER_H

#include <string>

//clase que compila el programa y devuelve su compilador en un string

class Compiler {
public:
    std::string compile(const std::string& filename);
};

#endif