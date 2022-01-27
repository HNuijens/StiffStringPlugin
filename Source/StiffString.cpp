/*
  ==============================================================================

    StiffString.cpp
    Created: 27 Jan 2022 10:24:42am
    Author:  Helmer Nuijens

  ==============================================================================
*/

#include "StiffString.h"


StiffString::StiffString()
{
   
}

StiffString::~StiffString()
{

}

void StiffString::setFs(double Fs)
{
    this->Fs = Fs;
    k = 1.0 / Fs; 
}

void StiffString::setGrid(NamedValueSet& parameters)
{
    // Get Parameters
    f0 = *parameters.getVarPointer("f0");
    L = *parameters.getVarPointer("L");
    rho = *parameters.getVarPointer("rho");
    r = *parameters.getVarPointer("r");
    E = *parameters.getVarPointer("E");
    sig0 = *parameters.getVarPointer("sig0");
    sig1 = *parameters.getVarPointer("sig1");

    A = r * r * double_Pi;
    I = r * r * r * r * double_Pi * 0.25;
    c = f0 * 2.0 * L;

    // Create Grid:
    kappaSq = E * I / rho * A;
    double stabTmp = c * c * k * k + 4.0 * sig1 * k;
    h = sqrt(0.5 * (stabTmp + sqrt(stabTmp * stabTmp + 16.0 * kappaSq * k * k)));
    N = floor(L / h);
    h = L / N;

    uStates.clear();
    uStates = vector<vector<double>>(3, vector<double>(N + 1, 0));

    u.clear();
    u = vector<double*>(3, nullptr);

    for (int i = 0; i < u.size(); ++i)
        u[i] = &uStates[i][0];


    // Calculate Stencil factors:
    lambdaSq = k * k * c * c / (h * h);
    S0 = sig0 * k;                              // freq ind damping factor
    S1 = 2 * k * sig1 / (h * h);                // freq dep damping factor
    K = -kappaSq * k * k / (h * h * h * h);     // stiffness factor
    D = 1 / (1 + S0);                           // fraction due to freq. indep. damping term

    G0_0 = (2 + -2 * lambdaSq + 6 * K - 2 * S1) * D;    // u_l^ n
    G0_1 = (lambdaSq - 4 * K + S1) * D;                 // u_l -/+1 ^ n
    G0_2 = K * D;                                       // u_l -/+2 ^ n
    G1_0 = (-1 + S0 + 2 * S1) * D;                      // u_l^ n - 1
    G1_1 = -S1 * D;                                     // u_l -/+1 ^ n - 1
}

double StiffString::getNextSample(float outputPos)
{
    // update the states
    calculateScheme();

    double out = u[0][static_cast<int> (round(outputPos * N))];

    updateStates();

    return out; 
}

void StiffString::calculateScheme()
{
    for (int l = 2; l < N - 1; l++)
    {
        u[0][l] = G0_0 * u[1][l] + G0_1 * (u[1][l - 1] + u[1][l + 1]) + G0_2 * (u[1][l - 2] + u[1][l + 2]) 
            + G1_0 * u[2][l] + G1_1 * (u[2][l - 1] + u[2][l + 1]);
    }
}

void StiffString::updateStates()
{
    double* uTmp = u[2];
    u[2] = u[1];
    u[1] = u[0];
    u[0] = uTmp;
}

void StiffString::exciteSystem()
{
    u[1][2] = 1;
    u[2][2] = 1;
   
}