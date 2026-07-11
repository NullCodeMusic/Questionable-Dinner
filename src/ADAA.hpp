#pragma once
#include "rack.hpp"
#include "mymath.hpp"

//https://quadst.rip/ln-approx, but I removed the random useless variable!
inline float quickln(float x) {
    unsigned int bx = * (unsigned int *) (&x);
    unsigned int ex = bx >> 23;
    signed int t = (signed int)ex-(signed int)127;
    bx = 1065353216 | (bx & 8388607);
    x = * (float *) (&bx);
    return -1.7417939+(2.8212026+(-1.4699568+(0.44717955-0.056570851*x)*x)*x)*x+0.6931471806*t;
}

struct ADAAClipF4Reinhard {
    rack::simd::float_4 tol = 1.0e-5f;
    //The plan is to use a piecewise function, and deal with rectifying and scaling when piecing stuff together AT THE VERY END!
    rack::simd::float_4 prev = 0;
    rack::simd::float_4 out = 0;
    rack::simd::float_4 o = 0;
    rack::simd::float_4 k = 0;

    ADAAClipF4Reinhard(rack::simd::float_4 k = 0){
        setK(k);
    }

    //See float version for clipping function. K must be >=0 and <1
    inline rack::simd::float_4 F1(rack::simd::float_4 x){
        x=abs(x);
        rack::simd::float_4 k1 = 1-k;
        rack::simd::float_4 axk = abs(x - k);
        return simd::ifelse(
            x<k, 
            x*x/2.f, 
            k1 * (-k1 * log( axk+k1) + axk)+k*x+o);
    }

    //k must be >=0 and <1
    inline void setK(rack::simd::float_4 k){
        rack::simd::float_4 k1 = 1-k;
        o = - k1 * (-k1 * log(k1))-k*k/2;
        this->k = k;
    }

    //k must be >=0 and <1
    inline void process(rack::simd::float_4 x,rack::simd::float_4 k){
        setK(k);
        //Falls back to previous value instead of calculating a new one
        rack::simd::float_4 diff = x - prev;
        out = simd::ifelse(abs(diff)>tol, ((F1(x)-F1(prev))/diff), out);
        prev = x;
    }

    //If you want to set k elsewhere
    inline void process(rack::simd::float_4 x){
        //Falls back to previous value instead of calculating a new one
        rack::simd::float_4 diff = x - prev;
        out = simd::ifelse(abs(diff)>tol, ((F1(x)-F1(prev))/diff), out);
        prev = x;
    }
};

struct ADAAClipFReinhard {
    float tol = 1.0e-5f;
    //The plan is to use a piecewise function, and deal with rectifying and scaling when piecing stuff together AT THE VERY END!
    float prev = 0;
    float out = 0;
    float o = 0;
    float k = 0;

    ADAAClipFReinhard(float k = 0){
        setK(k);
    }

    inline float f(float x,float k){
        return (x>-k&&x<k ? 
            x : 
            (
                x>0 ?
                (x-k)/(1+abs(x-k)/(1-k)):
                (x+k)/(1+abs(x+k)/(1-k))
            )+
            sgn(x)*k
        );
    }
    //Integral of the above clipping function. K must be >=0 and <1
    inline float F1(float x){
        x=abs(x);
        float k1 = 1-k;
        float axk = abs(x - k);
        return (x<k ?
            x*x/2.f //The centered linear portion beneath the knee
            :
            (
                k1 * (-k1 * quickln( axk+k1) + axk)+k*x //The clipping function
                +
                o //Constant offset to make it continuous
            ));
    }

    //k must be >=0 and <1
    inline void setK(float k){
        if(this->k==k){return;}
        float k1 = 1-k;
        o = - k1 * (-k1 * quickln(k1))-k*k/2;
        this->k = k;
    }

    //k must be >=0 and <1
    inline void process(float x,float k){
        setK(k);
        //Falls back to previous value instead of calculating a new one
        float diff = x - prev;
        out = abs(diff)>tol ? ((F1(x)-F1(prev))/diff) : out;
        prev = x;
    }

    //If you want to set k elsewhere
    inline void process(float x){
        //Falls back to previous value instead of calculating a new one
        float diff = x - prev;
        out = abs(diff)>tol ? ((F1(x)-F1(prev))/diff) : out;
        prev = x;
    }
};

struct ADAAClipF4Parabola {
    simd::float_4 tol = 1.0e-5f;
    //The plan is to use a piecewise function, and deal with rectifying and scaling when piecing stuff together AT THE VERY END!
    simd::float_4 prev = 0;
    simd::float_4 out = 0;
    simd::float_4 o = -2.f/3.f;
    simd::float_4 k = 0;

    ADAAClipF4Parabola(simd::float_4 k = 0){
        setK(k);
    }
    //Integral of a parabolic clip. K must be >=0 and <1
    inline simd::float_4 F1(simd::float_4 x){
        x=abs(x);
        simd::float_4 k1 = 1-k;
        simd::float_4 k2 = -(x-k)/k1+2;
        return simd::ifelse(x<k ,
            x*x/2.f //The centered linear portion beneath the knee
            ,
            (
                (x<2.f-k) & (k2*k2*k2*k1*k1/12.f) //The clipping function
            ) + o + x //Constant offset to make it continuous & flat
            );
    }

    //k must be >=0 and <1
    inline void setK(simd::float_4 k){
        o = (-k*k/2+k-2)/3;
        this->k = k;
    }

    //k must be >=0 and <1
    inline void process(simd::float_4 x,simd::float_4 k){
        setK(k);
        //Falls back to previous value instead of calculating a new one
        simd::float_4 diff = x - prev;
        out = simd::ifelse(abs(diff)>tol, ((F1(x)-F1(prev))/diff), out);
        prev = x;
    }

    //If you want to set k elsewhere
    inline void process(simd::float_4 x){
        //Falls back to previous value instead of calculating a new one
        simd::float_4 diff = x - prev;
        out = simd::ifelse(abs(diff)>tol, ((F1(x)-F1(prev))/diff), out);
        prev = x;
    }
};

struct ADAAClipFParabola {
    float tol = 1.0e-5f;
    //The plan is to use a piecewise function, and deal with rectifying and scaling when piecing stuff together AT THE VERY END!
    float prev = 0;
    float out = 0;
    float o = -2.f/3.f;
    float k = 0;

    ADAAClipFParabola(float k = 0){
        setK(k);
    }
    //Integral of a parabolic clip. K must be >=0 and <1
    inline float F1(float x){
        x=abs(x);
        float k1 = 1-k;
        float k2 = -(x-k)/k1+2;
        return (x<k ?
            x*x/2.f //The centered linear portion beneath the knee
            :
            (
                x<2.f-k ?
                (k2*k2*k2*k1*k1/12.f) //The clipping function
                :
                0//Flat
            ) + o + x //Constant offset to make it continuous & flat
            );
    }

    //k must be >=0 and <1
    inline void setK(float k){
        o = (-k*k/2+k-2)/3;
        this->k = k;
    }

    //k must be >=0 and <1
    inline void process(float x,float k){
        setK(k);
        //Falls back to previous value instead of calculating a new one
        float diff = x - prev;
        out = abs(diff)>tol ? ((F1(x)-F1(prev))/diff) : out;
        prev = x;
    }

    //If you want to set k elsewhere
    inline void process(float x){
        //Falls back to previous value instead of calculating a new one
        float diff = x - prev;
        out = abs(diff)>tol ? ((F1(x)-F1(prev))/diff) : out;
        prev = x;
    }
};

//Approximate Tanh Clipping
struct ADAAClipFTanh {
    float tol = 1.0e-5f;
    //The plan is to use a piecewise function, and deal with rectifying and scaling when piecing stuff together AT THE VERY END!
    float prev = 0;
    float out = 0;
    float o = -(quickln(2));
    float k = 0;

    ADAAClipFTanh(float k = 0){
        setK(k);
    }
    //Integral of a parabolic clip. K must be >=0 and <1
    inline float F1(float x){
        x=abs(x);
        float k1 = 1-k;
        float k2 = -(x-k)/k1;
        return (x<k ?
            x*x/2.f //The centered linear portion beneath the knee
            :
            (
                (quickln(exp2(2.88539008178f*k2)+1)-k2)*k1*k1+x*k
            ) + o//Constant offset to make it continuous & flat
            );
    }

    //k must be >=0 and <1
    inline void setK(float k){
        if(this->k==k){return;}
        o = -k*k/2.f -(quickln(2))*(1-k)*(1-k);
        this->k = k;
    }

    //k must be >=0 and <1
    inline void process(float x,float k){
        setK(k);
        //Falls back to previous value instead of calculating a new one
        float diff = x - prev;
        out = abs(diff)>tol ? ((F1(x)-F1(prev))/diff) : out;
        prev = x;
    }

    //If you want to set k elsewhere
    inline void process(float x){
        //Falls back to previous value instead of calculating a new one
        float diff = x - prev;
        out = abs(diff)>tol ? ((F1(x)-F1(prev))/diff) : out;
        prev = x;
    }
};

//Approximate Tanh Clipping. VCV's approximations are terrible for large numbers so this one's pretty slow.
struct ADAAClipF4Tanh {
    simd::float_4 tol = 1.0e-5f;
    //The plan is to use a piecewise function, and deal with rectifying and scaling when piecing stuff together AT THE VERY END!
    simd::float_4 prev = 0;
    simd::float_4 out = 0;
    simd::float_4 o = -(log(2));
    simd::float_4 k = 0;

    ADAAClipF4Tanh(simd::float_4 k = 0){
        setK(k);
    }
    //Integral of a parabolic clip. K must be >=0 and <1
    inline simd::float_4 F1(simd::float_4 x){
        x=abs(x);
        simd::float_4 k1 = 1-k;
        simd::float_4 k2 = -(x-k)/k1;
        return simd::ifelse(x<k,
            x*x/2.f //The centered linear portion beneath the knee
            ,
            (
                (log(exp(2*k2)+1)-k2)*k1*k1+x*k
            ) + o//Constant offset to make it continuous & flat
            );
    }

    //k must be >=0 and <1
    inline void setK(simd::float_4 k){
        o = -k*k/2.f -(quickln(2.f))*(1.f-k)*(1.f-k);
        this->k = k;
    }

    //k must be >=0 and <1
    inline void process(simd::float_4 x,simd::float_4 k){
        setK(k);
        //Falls back to previous value instead of calculating a new one
        simd::float_4 diff = x - prev;
        out = simd::ifelse(abs(diff)>tol , ((F1(x)-F1(prev))/diff) , out);
        prev = x;
    }

    //If you want to set k elsewhere
    inline void process(simd::float_4 x){
        //Falls back to previous value instead of calculating a new one
        simd::float_4 diff = x - prev;
        out = simd::ifelse(abs(diff)>tol , ((F1(x)-F1(prev))/diff) , out);
        prev = x;
    }
};