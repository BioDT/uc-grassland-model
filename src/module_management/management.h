#pragma once
#include <vector>
#include <iostream>

class MANAGEMENT
{
public:
    MANAGEMENT();
    ~MANAGEMENT();

    std::vector<int> mowingDate;
    std::vector<double> mowingHeight;

    std::vector<int> fertilizationDate;
    std::vector<double> fertilizerAmount;

    std::vector<int> irrigationDate;
    std::vector<double> irrigationAmount;

    std::vector<int> sowingDate;
    std::vector<std::vector<int>> amountOfSownSeeds; // dynamic 2D vector of pft and sowing events with elements being the event-specific seed numbers sown
};