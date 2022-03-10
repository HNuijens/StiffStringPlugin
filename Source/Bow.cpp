/*
  ==============================================================================

    Bow.cpp
    Created: 3 Mar 2022 3:57:09pm
    Author:  Helmer Nuijens

  ==============================================================================
*/
#include "Bow.h"

Bow::Bow()
{
    fb = 1.0;             // Bow force in N
    Fb = 0.0;             // Bow force on string
    vb = 0.0;             // Bow velocity
    xb = 0.0;             // Bow position
    a = 100;              // frcition model scaler
    BM = sqrt(2.0 * a) * exp(0.5);  // Bow model
    maxIter = 100;      // Newton-Raphson  max number of iteration
    eps = 1e-7;         // Newton-Raphson threshold
}
Bow::~Bow()
{

}

void Bow::setBowParams(NamedValueSet& parameters)
{
    rho = *parameters.getVarPointer("rho");
    r = *parameters.getVarPointer("r");
    A = double_Pi * r * r;
    Fb = fb / (rho * A);

    sig0 = *parameters.getVarPointer("sig0");
    sig1 = *parameters.getVarPointer("sig1");
    k = *parameters.getVarPointer("k");
    Fs = 1.0 / k; 
    h = *parameters.getVarPointer("h");
    kappaSq = *parameters.getVarPointer("kappaSq");
    cSq = *parameters.getVarPointer("cSq");
    N = *parameters.getVarPointer("N");

    // Calculate intermediate grid values
    GI0_0 = (-2.0 / (k * k) + 2.0 * cSq / (h * h) + 6.0 * kappaSq / (h * h * h * h) + 4.0 * sig1 / (k * h * h));
    GI0_1 = (-cSq / (h * h) - 4.0 * kappaSq / (h * h * h * h) - 2.0 * sig1 / (k * h * h));
    GI0_2 = (kappaSq / (h * h * h * h));
    GI1_0 = (2.0 / (k * k) - 4.0 * sig1 / (k * h * h));
    GI1_1 = (2.0 * sig1 / (k * h * h));
}

void Bow::setExcitation(std::vector<double*>& u, float bowPosition, double bowVelocity)
{
    xb = floor(bowPosition * N); 
    if (xb > N - 2) xb = N - 2;
    if (xb  < 2) xb = 2;

    vb = bowVelocity;
    // vb = 0.2 * sin(12 * double_Pi * t / Fs);

    double b = vb * (2.0 / k + sig0 * 2.0) + GI0_0 * u[1][xb] + GI0_1 * u[1][xb + 1] + GI0_1 * u[1][xb - 1] +
        GI0_2 * u[1][xb + 2] + GI0_2 * u[1][xb - 2] +
        GI1_0 * u[2][xb] + GI1_1 * u[2][xb + 1] + GI1_1 * u[2][xb - 1];

    // Find relative velocity between the bow and string:
    vRel = NewtonRaphson(maxIter, eps, b);

    // Apply excitation
    double excitation = (k * k / (rho * A * h * (1.0 + sig0 * k))) * fb * sqrt(2.0 * a) * vRel * exp(-a * vRel * vRel + 0.5);
    u[0][xb] -= excitation; // add bow excitation to the grid point
    t++; 
}

double Bow::NewtonRaphson(int maxIterations, double threshold, double b)
{
    double vRelPrev = 0.0; 
    double vRel = 0.0; 

    for (int i = 0; i < maxIterations; i++)
    {
        double mu = sqrt(2.0 * a) * vRelPrev * exp(-a * vRelPrev * vRelPrev + 0.5);
        double g = g = vRelPrev * 2 / k + 2 * sig0 * vRelPrev + (1 / h) * Fb * BM * vRelPrev * exp(-a * vRelPrev * vRelPrev) + b;
        double dg_dvRel = 2 / k + 2 * sig0 + (1 / h) * Fb * BM * (1 - 2 * a * vRelPrev * vRelPrev) * exp(-a * vRelPrev * vRelPrev);
       
        vRel = vRelPrev - (g / dg_dvRel);

        if (std::abs(vRel - vRelPrev) < eps)
        {
            break; 
        }
        vRelPrev = vRel;
    }
    
    return vRel; 
}
