#pragma once

#include <cmath>
#include <deque>

//Sine approx stolen from VCV fundamentals, thanks Andrew!
template <typename T>
inline T sin_2pi_9(T x) {
	x-=floor(x);
	// Shift argument to [-1, 1]
	x = T(2) * x - T(1);
	T x2 = x * x;
	return x * (x2 - T(1)) * (T(3.1415211942925003) + x2 * (T(-2.0247734792732333) + x2 * (T(0.51749274223813413) + x2 * T(-0.063691093590695858))));
}

template <typename T> inline int sign(T val) {
    	return (T(0) < val) - (val < T(0));
	}

inline float magnitude(float x, float y){
		float out = sqrt(x*x+y*y);
		if(std::isnan(out)){
			return 0;
		}
		return out;
	}

inline float ratioFrom(float f){
	if(f<0){
		return 1/(1-f);
	}else{
		return f+1;
	}
}

inline float softClip(float val, float thresh, float hardness ){ //Hardness must be < 1.f
	const float valSgn = sign(val);
	const float a = thresh/(1.f+hardness);
	const float b = abs(val)*0.5f/a;
	const float c = std::min(b,1.0f);
	const float d = std::max(b-hardness,0.0f)*valSgn/(1.0f-hardness);

	const float out = (d*(1.f-c)+c*valSgn)*a+std::max(std::min(0.5f*val,hardness*a),-hardness*a);
	return out;
}

inline float softClip(float val, float thresh){
	const float c = std::min(abs(val)/thresh/2,1.0f);
	const float out = ((val/thresh/2)*(1.f-c)+c*sign(val))*thresh;

	return out;
}

inline float softFold(float val, float thresh){
	const float a = val/(1.754*thresh);
	const float out = val/(1+a*a*a*a);
	return out;
}

inline float softFold05(float val){
	const float a = val/1.754f;
	const float b = clamp(abs(val)/5,0.0f,1.0f);
	const float out = val/(1+a*a*a*a)*(1-b);
	return out;
}

inline float exp2Decay(float val, float time, float sr){
	const float factor = dsp::exp2_taylor5(-(1.f/time)/sr);
	return val*factor;
}

inline float mxyRound(float val, float shape){
	float power = dsp::exp2_taylor5(shape+1);
	if(shape>0){
		return pow(abs(sin_2pi_9(val/2)),power);
	}else{
		return 1-pow(abs(sin_2pi_9(val/2+0.25)),4/power);
	}
}
inline float mxySpike(float val, float shape){
	float power = dsp::exp2_taylor5(shape+1);
	if(shape<0){
		return pow(abs(sin_2pi_9(val/2)),power);
	}else{
		return 1-pow(abs(sin_2pi_9(val/2+0.25)),4/power);
	}
}

enum staqueModes{
	SM_STACK,
	SM_QUEUE
};

template <typename T>
struct staque
{
	static constexpr int MAX_DELAY = 2646000;

	std::deque<T> data = {};
	int mode = SM_STACK;

	void push(T val){
		data.push_front(val);
		if(data.size()>MAX_DELAY){
			data.pop_back();
		}
	}

	void pop(){
		if(mode==SM_STACK){
			data.pop_front();
		}else{
			data.pop_back();
		}
	}

	T top(){
		if(mode==SM_STACK){
			return data.front();
		}else{
			return data.back();
		}
	}

	bool empty(){
		return data.empty();
	}

	void clear(){
		data.clear();
	}

	void swap(staque *s){
		data.swap(s->data);
	}
};
