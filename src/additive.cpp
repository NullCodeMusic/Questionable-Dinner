#include "additive.hpp"

using namespace rack;
using namespace simd;
using namespace additive;

#include "mymath.hpp"

Algorithm::~Algorithm(){}

void Algorithm::process(float freq,float n,float deltaTime,float prev,float structure,float morph){
    n/=4;
    float_4 i = {1,2,3,4};
    float_4 pFreq;
    float_4 pAmp;
    for(int idx = 0; idx < n; idx++){
        pAmp = 1.f/i;
        partials[PI_AMP][idx] = pAmp;
        i+=4;
    }
}

void Algorithm::reset(){}

//ADDATIVE OSCILLATOR ALGORITHMS
//Basic <Base>
void Basic::process(float freq,float n,float deltaTime,float prev,float structure,float morph){
    n/=4;
    float_4 i = {1,2,3,4};
    for(int idx = 0; idx < n; idx++){
        partials[PI_FREQ][idx] = (1.f/i+(1.f-1.f/i)*dsp::exp2_taylor5(structure*2));
        partials[PI_AMP][idx] = (fmod(i,2.f)*(1.f+morph)+fmod((i+1),2)*(1.f-morph))/i;
        i+=4;
    }
}
//Organ <Orgn>
void Organ::process(float freq,float n,float deltaTime,float prev,float structure,float morph){
    //Original Surgeon Patch
    //j = (1+(1+x)/2)
    //k = (1+(1+y)/2)
    //freq = f*j^(i-1)*k^((i+1)%2)

    n/=4;
    float_4 i = {1,2,3,4};
    float temp = (1+(1+structure)/2);
    float_4 j = {1,temp,temp*temp,temp*temp*temp};
    float j_pow4 = j[2]*j[2];
    float_4 k = {1,(1+(1+morph)/2),1,(1+(1+morph)/2)};
    for(int idx = 0; idx < n; idx++){
        i+=4;
        j*=j_pow4;
        partials[PI_FREQ][idx] = j*k/i;
        partials[PI_AMP][idx] = 1.f/i;
    }
}
//Low-Qual <LowQ>
void LowQual::process(float freq,float n,float deltaTime,float prev,float structure,float morph){
    //Original Surgeon Patch
    //j = 100^(1+x)+0.0001
    //k = j*(y+1)
    //freq = f+floor(f*(i-1)/j)*k

    n/=4;
    float_4 i = {1,2,3,4};
    float j = dsp::exp2_taylor5((1+structure)*5);
    time+=1-deltaTime;
    if(time>1000){
        time-=1000;
    }
    float k = 1+int(time*(2+morph))/(2+morph)-time;
    for(int idx = 0; idx < n; idx++){
        partials[PI_PHASE][idx] = k;
        partials[PI_FREQ][idx] = (1+int32_4(freq*(i-1)/j)*j/freq)/i;
        partials[PI_AMP][idx] = 1.f/i;
        i+=4;
    }
}
void LowQual::reset(){
    time = 0;
}
//Fish <}{{D> (deep sea in presets)
void Fish::process(float freq,float n,float deltaTime,float prev,float structure,float morph){
    //Original Surgeon Patch
    //j = (i-1)/n*6*f*(x+y+2)
    //k = (i-1)/n*6*f*(x-y+2)
    //freq = f+j*(i%2)+k*((i+1)%2)
    float_4 i = {1,2,3,4};
    float_4 j;
    float_4 k;
    float amp = 1.f/n;
    for(int idx = 0; idx*4 < n; idx++){
        j = (i-1)*6*(structure+morph+1)*amp;
        k = (i-1)*6*(structure-morph+1)*amp;
        partials[PI_FREQ][idx] = (1+j*fmod(i,2)+k*fmod((i+1),2))/i;
        partials[PI_AMP][idx] = float_4(amp)*2;
        i+=4;
    }
}
//Combed Saw <Comb>
inline float_4 triangle(float_4 x){
    float_4 a = fmod(x,2);
    return clamp(a,0,1)-clamp(a,1,2)+1;
}
inline float triangle(float x){
    float a = fmod(x,2);
    return clamp(a,0.f,1.f)-clamp(a,1.f,2.f)+1;
}
void CombedSaw::process(float freq,float n,float deltaTime,float prev,float structure,float morph){
    //Original Surgeon Patch
    //j = cos(i*10*x)*(y+1)/2
    //k = sin(i*10*x)*(1-y)/2
    //a = 1/i*e*(j^2+k^2)
    //Find better than sin^2 and cos^2

    n/=4;
    float_4 i = {1,2,3,4};
    float_4 j;
    float_4 k;
    for(int idx = 0; idx < n; idx++){
        j = triangle(i*6*(1-structure)/n);
        k = 1-j;
        partials[PI_FREQ][idx] = 1;
        partials[PI_AMP][idx] = 1/i * (j*(1+morph)+k*(1-morph));
        i+=4;
    }
}
//Metallic <Metl>
void Metallic::process(float freq,float n,float deltaTime,float prev,float structure,float morph){
    //based on an 18 in crash sample analyzed by hand
    //
    n/=4;
    float_4 i = {1,2,3,4};
    
    for(int idx = 0; idx < n; idx++){
        float_4 temp = (i-2.87-(1+morph)*n*2)/2.1;
        partials[PI_FREQ][idx] = 1+(dsp::exp2_taylor5(clamp(((0.000394669*i*i*i)-(0.0140304*i*i)+(0.155518*i)-(0.211863)+0.0699807/i)-1,-30,30)))*structure;
        partials[PI_AMP][idx] = clamp(-temp*temp+1,i/150-0.02,1);
        //partials[PI_PHASE][idx] = 0.25;
        i+=4;
    }
}
//Fractal <Frct>
void Fractal::process(float freq,float n,float deltaTime,float prev,float structure,float morph){
    //
    n/=4;
    float_4 i = {1,2,3,4};
    structure++;
    morph++;
    float_4 j = {
        1,
        1+morph,
        1+morph+morph,
        1+morph+morph+morph
    };
    for(int idx = 0; idx < n; idx++){
        partials[PI_FREQ][idx] = j/i;
        partials[PI_AMP][idx] = 1/i;
        i+=4;
        j+=structure*4;
    }
}
//Particles <;*.:> 
void Particles::process(float freq,float n,float deltaTime,float prev,float structure,float morph){
    //Original Surgeon Patch
    //j = 1-((t*20)%i^x)/i^x
    //freq = i*f+2*f*j^8*y
    //a = 1/i*e*j
    time+=deltaTime;
    n/=4;
    float_4 i = {1,2,3,4};
    float s2 = structure*2;
    if(s2==0){
        return;
    }
    float_4 j;
    if(time>abs(s2)){
        time-=abs(s2);
    }
    for(int idx = 0; idx < n; idx++){
        j = 1-fmod((time*20),i*s2)/(i*s2);
        partials[PI_FREQ][idx] = 1+2*pow(j,8)*morph/i;
        partials[PI_AMP][idx] = 1/i*j;
        i+=4;
    }
}
void Particles::reset(){
    time = 0;
}
//Prism <Prsm> (detuner in presets) 
void Prism::process(float freq,float n,float deltaTime,float prev,float structure,float morph){
    //Original Surgeon Patch
    n/=4;
    float_4 i = {1,2,3,4};
    float_4 j;
    for(int idx = 0; idx < n; idx++){
        j = triangle(i*6*(1-structure)/n)+morph;
        partials[PI_FREQ][idx] = 1+j;
        partials[PI_AMP][idx] = 1/i;
        i+=4;
    }
}
//Chaotic <Dice> (shimmer in presets)
inline float_4 rand4(){
    return float_4(rand()%1000,rand()%1000,rand()%1000,rand()%1000)/1000.f;
}
void Chaotic::process(float freq,float n,float deltaTime,float prev,float structure,float morph){
    //Original Surgeon Patch
    //i*f*(1-r*z)

    //Structure to rand seed, morph is z
    n/=4;
    float_4 i = {1,2,3,4};
    srand(structure*1000);
    for(int idx = 0; idx < n; idx++){
        partials[PI_FREQ][idx] = 1+rand4()*morph;
        partials[PI_AMP][idx] = 1/i;
        i+=4;
    }
}
//Noise <&#$%> (wind or robot in presets)
void Noise::process(float freq,float n,float deltaTime,float prev,float structure,float morph){
    //Original Surgeon Patch
    //j = ((1+z)/2)^3
    //k = 1-j
    //freq = r*20000*(e^4+y+1)*j+f*k
    //a = i/n^2*5
    //phase = r

    //Remove envelope!

    n/=4;
    float_4 i = {1,2,3,4};
    float lerp = (morph+1)/2;
    srand(time);
    if(random::u32()%1000<=dsp::exp2_taylor5((structure+1)*5)){
        time++;
    }
    float_4 r;
    for(int idx = 0; idx < n; idx++){
        r = rand4();
        partials[PI_FREQ][idx] = 1+20000*r*lerp/i/freq-lerp;
        partials[PI_AMP][idx] = 1/n;
        partials[PI_PHASE][idx] = r;
        i+=4;
    }
}
void Noise::reset(){
    time = 0;
}
//Phase Mod <PMod>
void PhaseMod::process(float freq,float n,float deltaTime,float prev,float structure,float morph){
    //Original Surgeon Patch
    //j = 3^(y+1)-1
    //k = sin(t*3.1415*f)*j
    //phase = out*x*0.08+k
    time += deltaTime*freq;
    time -= int(time);
    n/=4;
    float_4 i = {1,2,3,4};
    float j = (structure+1.f)*2;
    float k = sin_2pi_9(time)*j;
    for(int idx = 0; idx < n; idx++){
        partials[PI_FREQ][idx] = 1.f;
        partials[PI_AMP][idx] = 1.f/i;
        partials[PI_PHASE][idx] = k*i+morph/2.f;
        i+=4;
    }
}
void PhaseMod::reset(){
    time = 0;
}
//Per Partial AM <PPAM>
void PerPartialAM::process(float freq,float n,float deltaTime,float prev,float structure,float morph){
    //Original Surgeon Patch
    //j = (1+z)*4
    //k = sin(i*f*pi*t*j)*(1+y)
    //freq = i*f+f*k*4
    n/=4;
    float_4 i = {1,2,3,4};
    float j = (1.f+structure)*4.f;
    time += deltaTime*j;
    time -= int(time/10.f)*10.f;
    float_4 k;
    for(int idx = 0; idx < n; idx++){
        k = triangle(i*freq*2*time)*(1+morph);
        partials[PI_FREQ][idx] = 1+morph;
        partials[PI_AMP][idx] = 1/i*k;
        i+=4;
    }
}
void PerPartialAM::reset(){
    time = 0;
}
//PianoBrass <Keys>
constexpr static float pianoFreq[128] = {
    1, 0.99654908, 0.99718814, 1.0053681, 1.0041411, 1.0043456, 1.0062445, 1.002684,
    1.0063054, 1.0076687, 1.0073898, 1.0116309, 1.01168, 1.0856705, 1.2295501, 1.2102186,
    1.1476001, 1.3164622, 1.4865192, 1.6265337, 1.9810838, 1.9900307, 2.1712457, 2.23085514,
    2.32357114, 2.42148714, 2.52460314, 2.63291914, 2.74643514, 2.86515114, 2.98906714, 3.11818314,
    3.25249914, 3.39201514, 3.53673114, 3.68664714, 3.84176314, 4.00207914, 4.16759514, 4.33831114,
    4.51422714, 4.69534314, 4.88165914, 5.07317514, 5.26989114, 5.47180714, 5.67892314, 5.89123914,
    6.10875514, 6.33147114, 6.55938714, 6.79250314, 7.03081914, 7.27433514, 7.52305114, 7.77696714,
    8.03608314, 8.30039914, 8.56991514, 8.84463114, 9.12454714, 9.40966314, 9.69997914, 9.99549514,
    10.29621114, 10.60212714, 10.91324314, 11.22955914, 11.55107514, 11.87779114, 12.20970714, 12.54682314,
    12.88913914, 13.23665514, 13.58937114, 13.94728714, 14.31040314, 14.67871914, 15.05223514, 15.43095114,
    15.81486714, 16.20398314, 16.59829914, 16.99781514, 17.40253114, 17.81244714, 18.22756314, 18.64787914,
    19.07339514, 19.50411114, 19.94002714, 20.38114314, 20.82745914, 21.27897514, 21.73569114, 22.19760714,
    22.66472314, 23.13703914, 23.61455514, 24.09727114, 24.58518714, 25.07830314, 25.57661914, 26.08013514,
    26.58885114, 27.10276714, 27.62188314, 28.14619914, 28.67571514, 29.21043114, 29.75034714, 30.29546314,
    30.84577914, 31.40129514, 31.96201114, 32.52792714, 33.09904314, 33.67535914, 34.25687514, 34.84359114,
    35.43550714, 36.03262314, 36.63493914, 37.24245514, 37.85517114, 38.47308714, 39.09620314, 39.72451914
};
constexpr static float westernFreq[128] = {
    0.964758046,0.9983946181,1.006039294,0.9813852152,0.9893739011,0.9866727824,1.11939891,1.116122621,
    1.099134453,1.203271921,1.191179434,1.244807991,1.525406785,1.47815261,1.557984864,1.780253803,
    1.869797686,2.106957504,2.303059077,2.517009403,2.52460314,2.63291914,2.74643514,2.86515114,
    2.98906714,3.11818314,3.25249914,3.39201514,3.53673114,3.68664714,3.84176314,4.00207914,
    4.16759514,4.33831114,4.51422714,4.69534314,4.88165914,5.07317514,5.26989114,5.47180714,
    5.67892314,5.89123914,6.10875514,6.33147114,6.55938714,6.79250314,7.03081914,7.27433514,
    7.52305114,7.77696714,8.03608314,8.30039914,8.56991514,8.84463114,9.12454714,9.40966314,
    9.69997914,9.99549514,10.29621114,10.60212714,10.91324314,11.22955914,11.55107514,11.87779114,
    12.20970714,12.54682314,12.88913914,13.23665514,13.58937114,13.94728714,14.31040314,14.67871914,
    15.05223514,15.43095114,15.81486714,16.20398314,16.59829914,16.99781514,17.40253114,17.81244714,
    18.22756314,18.64787914,19.07339514,19.50411114,19.94002714,20.38114314,20.82745914,21.27897514,
    21.73569114,22.19760714,22.66472314,23.13703914,23.61455514,24.09727114,24.58518714,25.07830314,
    25.57661914,26.08013514,26.58885114,27.10276714,27.62188314,28.14619914,28.67571514,29.21043114,
    29.75034714,30.29546314,30.84577914,31.40129514,31.96201114,32.52792714,33.09904314,33.67535914,
    34.25687514,34.84359114,35.43550714,36.03262314,36.63493914,37.24245514,37.85517114,38.47308714,
    39.09620314,39.72451914,40.35803514,40.99675114,41.64066714,1.48501914,1.48501914,1.48501914,
};
constexpr static float pianoAmp[128] = {
    0.9141132415, 2, 0.1581689584, 1.5, 0.4666271504, 0.2138706801, 0.2213594362, 0.03184857364,
    0.02846049894, 0.1733803998, 0.008737610582, 0.03134593625, 0.01636603035, 0.07518445149, 0.01545579181, 0.02748653419,
    0.02362919473, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125,
    0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125,
    0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125,
    0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125,
    0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125,
    0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125,
    0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125,
    0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125,
    0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125,
    0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125,
    0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125,
    0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125,
    0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125,
    0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125, 0.03186196125
};

void PianoBrass::process(float freq,float n,float deltaTime,float prev,float structure,float morph){
    //Analyze piano, e-piano, sax, and trumpet samples by hand, blend coeffs using morph and structure.
    n/=4;
    float_4 i = {1,2,3,4};
    float_4 filter;
    float lerp1 = structure*1.05;
    float lerp2 = morph*morph*morph;
    for(int idx = 0; idx < n; idx++){
        partials[PI_FREQ][idx] = (
            float_4(pianoFreq[(int)i[0]-1],pianoFreq[(int)i[1]-1],pianoFreq[(int)i[2]-1],pianoFreq[(int)i[3]-1])
            *lerp1
            +float_4(westernFreq[(int)i[0]-1],westernFreq[(int)i[1]-1],westernFreq[(int)i[2]-1],westernFreq[(int)i[3]-1])
            *(1-lerp1))
            +fmod(i,2)*lerp2;
        filter = 1-clamp((partials[PI_FREQ][idx]-3090)/5410);
        partials[PI_AMP][idx] = float_4(pianoAmp[(int)i[0]-1],pianoAmp[(int)i[1]-1],pianoAmp[(int)i[2]-1],pianoAmp[(int)i[3]-1])/i*filter;
        i+=4;
    }
}