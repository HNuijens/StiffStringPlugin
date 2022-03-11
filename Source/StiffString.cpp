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
    kappaSq = E * I / (rho * A);
    double stabTmp = c * c * k * k + 4.0 * sig1 * k;
    h = sqrt(0.5 * (stabTmp + sqrt((stabTmp * stabTmp) + 16.0 * kappaSq * k * k)));
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

    G0_0 = (2 -2 * lambdaSq + 6 * K - 2 * S1) * D;      // u_l^ n
    G0_1 = (lambdaSq - 4 * K + S1) * D;                 // u_l -/+1 ^ n
    G0_2 = K * D;                                       // u_l -/+2 ^ n
    G1_0 = (-1 + S0 + 2 * S1) * D;                      // u_l^ n - 1
    G1_1 = -S1 * D;                                     // u_l -/+1 ^ n - 1

    // Add bow parameters
    bowParameters = parameters; 
    bowParameters.set("kappaSq", kappaSq);
    bowParameters.set("cSq", c * c);
    bowParameters.set("h", h);
    bowParameters.set("k", k);
    bowParameters.set("N", N);

    bow.setBowParams(bowParameters);
}

double StiffString::getNextSample(float outputPos)
{
    calculateScheme();

    double out = u[0][static_cast<int> (round(outputPos * N))];
    out = out * eScalar;  // scale to make excitaiton audible 
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

    // Simply supported boundery condition: 
    u[0][1] = G0_0 * u[1][1] + G0_1 * (u[1][2]) + G0_2 * (u[1][3])
        + G1_0 * u[2][1] + G1_1 * (u[2][2]);
    u[0][N - 1] = G0_0 * u[1][N - 1] + G0_1 * (u[1][N - 2]) + G0_2 * (u[1][N - 3])
        + G1_0 * u[2][N - 1] + G1_1 * (u[2][N - 2]);

    // Bow string
    if(bowed)bow.setExcitation(u, ePos, vb);
}

void StiffString::updateStates()
{
    // Pointer switch
    double* uTmp = u[2];
    u[2] = u[1];
    u[1] = u[0];
    u[0] = uTmp;
}

void StiffString::exciteSystem(double amp, float pos, int width, bool strike)
{
    //// Excitation using a Hann window/ raised cosine ////
    
    if (strike) amp *= 1.0 / (width * 0.7); // prevent distortion of sound when striking
    if (amp > 1.0) amp = 1.0; 

    int startPos = floor(pos * N - width * 0.5);
    if (startPos < 1) startPos = 1;
    for (int w = startPos; w < startPos + width; w++)
    {
        if (w > (N - 2)) break;
        else
        {
            u[2][w] = 0.5 * amp * (1 - cos((2 * double_Pi * (w - startPos)) / width)) / eScalar;
            if (!strike)
                u[1][w] = u[2][w];
            ;
        }
    }
}