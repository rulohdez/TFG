#ifndef GASCALCULATOR_H
#define GASCALCULATOR_H

#include <vector>
#include "Instruction.h"

class GasCalculator {
public:
    int calculate(const std::vector<Instruction>& instructions);
};

#endif
