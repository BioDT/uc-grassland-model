#pragma once

const double tolerance = 0.0001;
const double PI = 3.14159265358979323846;
const double carbonContentOdm = 0.44;            // 1 g ODM contains 0.44 g Carbon
const double CO2ConversionToOdm = 0.63;          // ODM in 1 g CO2
const double molarMassOfCO2 = 44E-6;             // g CO2 in 1 µmol CO2
const double lightTransmissionCoefficient = 0.1; // TODO: unit???
const int heightLayerWidth = 1;                  // (cm)
const int maximumHeightLayer = 5000;             // equals 300 cm height with heightLayerWidth = 1 cm
const int soilLayerWidth = 10;                   // (cm)
const int maximumSoilLayer = 20;                 // equals 200 cm soil depth with soilLayerWidth = 10 cm
