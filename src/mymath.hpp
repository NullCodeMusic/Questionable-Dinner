#pragma once
#include <cmath>
#include <deque>

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

enum staqueModes{
	SM_STACK,
	SM_QUEUE
};

float ratioFrom(float f){
	if(f<0){
		return 1/(1-f);
	}else{
		return f+1;
	}
}

static constexpr int MAX_DELAY = 2646000;

template <typename T>
struct staque
{
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
