/*
  ==============================================================================

    StiffString.h
    Created: 27 Jan 2022 10:24:42am
    Author:  Helmer Nuijens

  ==============================================================================
*/

#include <JuceHeader.h>
#include <cmath>
#include <vector>
using namespace std;

#pragma once

class StiffString
{

public:
    StiffString();      // Constructor
    ~StiffString();     // Destructor
    
    void setFs(double Fs);
    void setGrid(NamedValueSet& parameters);
    double getNextSample(float outputPos);
    void exciteSystem(double amp, float pos, int width, bool strike);
   
    double Fs = 48000.0;

private:
    void calculateScheme();
    void updateStates();
   


    double h, k, L, c, f0, r, A, I, E, rho, sig0, sig1, kappaSq, lambdaSq;  // parameters
    double S0, S1, K, D, G0_0, G0_1, G0_2, G1_0, G1_1;                      // Stencil factors
    int N;                                                                  // grid size

    vector<vector<double>> uStates;                                         // vector containing the grid states
    vector<double*> u;                                                      // vector with pointers to the grid states

};