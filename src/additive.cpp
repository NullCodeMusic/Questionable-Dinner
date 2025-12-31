#include "additive.hpp"

using namespace rack;

Algorithm::~Algorithm(){}

float Algorithm::getPartialFreq(float freq,int i){return freq*i*partials[PI_FREQ][i];}

float Algorithm::getPartialAmp(float freq,int i){return partials[PI_AMP][i];}

float Algorithm::getPartialPhase(float freq,int i){return partials[PI_PHASE][i];}
    
float Algorithm::getModFreq(int i){return partials[PI_FREQ][i];}

void Algorithm::process(float freq,float n,float deltaTime,float env,float prev,float structure,float morph){}

void Algorithm::reset(){}

//ADDATIVE OSCILLATOR ALGORITHMS
//Test <Test>
void Test::process(float freq,float n,float deltaTime,float env,float prev,float structure,float morph){
    for(float i = 1; i <= n; i++){
        int idx = i-1;
        partials[PI_FREQ][idx] = 2;
        partials[PI_AMP][idx] = 1;
    }
}

//Basic <Base>
void Basic::process(float freq,float n,float deltaTime,float env,float prev,float structure,float morph){
    for(float i = 1; i <= n; i++){
        int idx = i-1;
        partials[PI_FREQ][idx] = (1.f/i+(1.f-1.f/i)*dsp::exp2_taylor5(structure*2));
        partials[PI_AMP][idx] = (fmod(i,2.f)*(1.f+morph)+fmod((i+1),2)*(1.f-morph))/i;
    }
}

//Organ <Orgn>
//Low-Qual <LowQ>
//Combed Saw <Comb>
//Metallic <Metl>
//Chord <Chrd>
//Particles <;*.:>
//Laser <Pew!>
//Prism <Prsm> (detuner in presets)
//Chaotic <Dice> (shimmer in presets)
//Random <&#$%> (wind or robot in presets)
//Phase Mod <PMod>
//Per Partial FM <PPFM>