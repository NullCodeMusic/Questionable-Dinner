#include <cmath>
#include <deque>
#include "plugin.hpp"
#include "mymath.hpp"

template <typename T> int sign(T val) {
    	return (T(0) < val) - (val < T(0));
	}

float magnitude(float x, float y){
		float out = sqrt(x*x+y*y);
		if(std::isnan(out)){
			return 0;
		}
		return out;
	}

float ratioFrom(float f){
	if(f<0){
		return 1/(1-f);
	}else{
		return f+1;
	}
}

float softClip(float val, float thresh, float hardness ){ //Hardness must be < 1.f
	const float valSgn = sign(val);
	const float a = thresh/(1.f+hardness);
	const float b = abs(val)*0.5f/a;
	const float c = std::min(b,1.0f);
	const float d = std::max(b-hardness,0.0f)*valSgn/(1.0f-hardness);

	const float out = (d*(1.f-c)+c*valSgn)*a+rack::math::clamp(0.5f*val,hardness*a,-hardness*a);
	return out;
}

float softClip(float val, float thresh){
	const float valSgn = sign(val);
	const float c = std::min(abs(val)/thresh/2,1.0f);
	const float d = val/thresh/2;

	const float out = (d*(1.f-c)+c*valSgn)*thresh;

	return out;
}

float softFold(float val, float thresh){
	const float a = val/(1.754*thresh);

	const float out = val/(1+a*a*a*a);

	return out;
}
