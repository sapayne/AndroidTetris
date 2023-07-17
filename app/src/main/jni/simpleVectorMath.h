//
// Created by Sapay on 4/8/2019.
//

#ifndef TUTORIAL05_TRIANGLE_SIMPLEVECTORMATH_H
#define TUTORIAL05_TRIANGLE_SIMPLEVECTORMATH_H

#include <math.h>
#include "vec2.h"

// dot product of two 2d vectors
double dot(vec2 &v1, vec2 &v2){
	return v1.x * v2.x + v1.y * v2.y;
}

// magnitude of a 2d vector
double mag(vec2 &v1){
	return sqrt(v1.x * v1.x + v1.y * v1.y);
}

// angle between two 2d vectors
double angle(vec2 &v1, vec2 &v2){
	double temp = mag(v1) * mag(v2);
	if(temp == 0){
		return 0;
	} else {
		return dot(v1, v2) / temp;
	}
}

double absAngle(vec2 &v1, vec2 &v2){
	return fabs(angle(v1,v2));
}

double dist(vec2 v1, vec2 v2){
	return sqrt(pow((v2.x - v1.x),2) + pow((v2.y - v1.y),2));
}

#endif //TUTORIAL05_TRIANGLE_SIMPLEVECTORMATH_H
