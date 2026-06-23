#pragma once
#include <rack.hpp>
#include "mymath.hpp"

static constexpr int MAX_PARTIALS = 32;//Divided by 4

enum PartialsInfo {
    PI_FREQ,
    PI_PHASE,
    PI_AMP
};
namespace additive{
struct Algorithm
{
    virtual ~Algorithm();
    rack::simd::float_4 partials[3][MAX_PARTIALS] = {{1},{1},{1}};
    std::string text = "0000";
    bool phaseResettable = true;

    virtual void process(float freq,float n,float deltaTime,float prev,float structure,float morph);
    virtual void reset();
};

//ADDATIVE OSCILLATOR ALGORITHMS
//Basic <Base>
struct Basic : Algorithm
{
    Basic(){
        text = "Base";
    }
    void process(float freq,float n,float deltaTime,float prev,float structure,float morph) override;
};

//Organ <Orgn>
struct Organ : Algorithm
{
    Organ(){
        text = "Orgn";
        phaseResettable = false;
    }
    void process(float freq,float n,float deltaTime,float prev,float structure,float morph) override;
};

//Low-Qual <LowQ>
struct LowQual : Algorithm
{
    float time;
    LowQual(){
        text = "LowQ";
    }
    void process(float freq,float n,float deltaTime,float prev,float structure,float morph) override;
    void reset() override;
};

//Fish <}{{D> (deep sea in presets)
struct Fish : Algorithm
{
    Fish(){
        text = "Fish";
    }
    void process(float freq,float n,float deltaTime,float prev,float structure,float morph) override;
};

//Combed Saw <Comb>
struct CombedSaw : Algorithm
{
    CombedSaw(){
        text = "Comb";
    }
    void process(float freq,float n,float deltaTime,float prev,float structure,float morph) override;
};

//Metallic <Metl>
struct Metallic : Algorithm
{
    Metallic(){
        text = "Metl";
    }
    void process(float freq,float n,float deltaTime,float prev,float structure,float morph) override;
};

//Fractal <Frct>
struct Fractal : Algorithm
{
    Fractal(){
        text = "Frct";
    }
    void process(float freq,float n,float deltaTime,float prev,float structure,float morph) override;
};

//Particles <;*.:>
struct Particles : Algorithm
{
    float time;
    Particles(){
        text = ";*.:";
    }
    void process(float freq,float n,float deltaTime,float prev,float structure,float morph) override;
    void reset() override;
};

struct PianoBrass : Algorithm 
{
    PianoBrass(){
        text = "Keys";
    }
    void process(float freq,float n,float deltaTime,float prev,float structure,float morph) override;
};

//Prism <Prsm> (detuner in presets)
struct Prism : Algorithm
{
    Prism(){
        text = "Prsm";
    }
    void process(float freq,float n,float deltaTime,float prev,float structure,float morph) override;
};

//Chaotic <Dice> (shimmer in presets)
struct Chaotic : Algorithm
{
    PRNGCache4<32, 1000> prng;
    Chaotic(){
        text = "Dice";
    }
    void process(float freq,float n,float deltaTime,float prev,float structure,float morph) override;
};

//Noise <&#$%> (wind or robot in presets)
struct Noise : Algorithm
{
    PRNGCache4<32, 1000> prng;
    unsigned long time = 0;
    Noise(){
        text = "&#$%";
    }
    void process(float freq,float n,float deltaTime,float prev,float structure,float morph) override;
    void reset() override;
};

//Phase Mod <PMod>
struct PhaseMod : Algorithm
{
    float time = 0;
    PhaseMod(){
        text = "PMod";
    }
    void process(float freq,float n,float deltaTime,float prev,float structure,float morph) override;
    void reset() override;
};

//Per Partial AM <PPAM>
struct PerPartialAM : Algorithm
{
    float time = 0;
    PerPartialAM(){
        text = "PPAM";
    }
    void process(float freq,float n,float deltaTime,float prev,float structure,float morph) override;
    void reset() override;
};

//BLEND FUNCTION
enum FTypes {
    FTYPE_HIGHPASS,
    FTYPE_LOWPASS,
    FTYPE_NOTCH,
    FTYPE_BANDPASS
};

struct BlendFilter {
    rack::simd::float_4 partials[MAX_PARTIALS];

    float center = 1; 
    float width = 0;
    float bias = 0; 
    float amount = 0;
    int type = FTYPE_LOWPASS;
    bool preserveFund;
    int dither = 0;
    float pSum = 0;

    void process();
};

namespace biohack{
///////////////////////////
//                       //
//  BIOHACK SYNTH CODE   //
//                       //
///////////////////////////
struct BaseSynth{
    virtual ~BaseSynth();
    simd::float_4 partials[3][MAX_PARTIALS] = {};
    simd::float_4 phasors[MAX_PARTIALS] = {};
    bool queuedReset;
    float sampleTime;
    simd::float_4 mask = {1,0,0,0};
    void setParams(float sampleRate, float numPartials){

    }
    void process(){

    }
};
///////////////////////////
//                       //
//  BIOHACK MORPHS CODE  //
//                       //
///////////////////////////
}

}