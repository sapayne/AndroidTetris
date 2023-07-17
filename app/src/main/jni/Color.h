//
// Created by Sapay on 4/6/2019.
//

#ifndef TUTORIAL05_TRIANGLE_COLOR_H
#define TUTORIAL05_TRIANGLE_COLOR_H


class Color {
private:
	float redValue, greenValue, blueValue;
public:
	Color(float red, float green, float blue){
		redValue = red;
		greenValue = green;
		blueValue = blue;
	};

	float getRed(){
		return Color::redValue;
	};
	float  getGreen(){
		return Color::greenValue;
	};
	float getBlue(){
		return Color::blueValue;
	};
};


#endif //TUTORIAL05_TRIANGLE_COLOR_H
