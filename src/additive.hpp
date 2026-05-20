#pragma once
#include <rack.hpp>

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
    Chaotic(){
        text = "Dice";
    }
    void process(float freq,float n,float deltaTime,float prev,float structure,float morph) override;
};

//Noise <&#$%> (wind or robot in presets)
struct Noise : Algorithm
{
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
}