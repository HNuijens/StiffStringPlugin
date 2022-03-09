/*
  ==============================================================================

    Bow.h
    Created: 3 Mar 2022 3:57:03pm
    Author:  Helmer Nuijens

  ==============================================================================
*/
#include <JuceHeader.h>
#include <vector>

class Bow
{

public:
    Bow();      // Constructor
    ~Bow();     // Destructor
    
    void setBowParams(NamedValueSet& parameters);
    void setExcitation(std::vector<double*>& u, float bowPosition, double bowVelocity);
    double NewtonRaphson(int maxIterations, double threshold, double b);

    double vb; 
private:
    double fb, Fb, vRel, a, eps;                    // Bow parameters
    int xb;                                             // Bow position
    double sig0, sig1, k, h, rho, r, A, kappaSq, cSq;   // Grid parameters
    double GI0_0, GI0_1, GI0_2, GI1_0, GI1_1;           // Intermediate grid values
    double Fs; 
    int t = 0; 
    int maxIter, N;

};
#pragma once


