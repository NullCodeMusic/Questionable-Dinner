#pragma once
#include <rack.hpp>

static constexpr int MAX_PARTIALS = 128;

enum PartialsInfo {
    PI_FREQ,
    PI_PHASE,
    PI_AMP
};

struct Algorithm
{
    virtual ~Algorithm();
    float partials[3][128] = {{1},{1},{1}};

    float getPartialFreq(float freq,int i);
    float getPartialAmp(float freq,int i);
    float getPartialPhase(float freq,int i);
    float getModFreq(int i);

    virtual void process(float freq,float n,float deltaTime,float env,float prev,float structure,float morph);
    virtual void reset();
};

struct Test : Algorithm 
{
    void process(float freq,float n,float deltaTime,float env,float prev,float structure,float morph) override;
};

struct Basic : Algorithm
{
    void process(float freq,float n,float deltaTime,float env,float prev,float structure,float morph) override;
};