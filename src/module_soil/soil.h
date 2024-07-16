#pragma once
#include <vector>
#include <iostream>

class SOIL
{
public:
    SOIL();
    ~SOIL();

    /* soil input parameter */
    double siltContent;
    double sandContent;
    double clayContent;
    std::vector<double> permanentWiltingPoint;
    std::vector<double> fieldCapacity;
    std::vector<double> porosity;
    std::vector<double> saturatedHydraulicConductivity;
};