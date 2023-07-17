//
// Created by Sapay on 4/6/2019.
//


#ifndef TUTORIAL05_TRIANGLE_BOARD_HPP
#define TUTORIAL05_TRIANGLE_BOARD_HPP
#include <vector>

class board {
private:
	int numOfFaces = 1, boardRowLength = 10;//, boardColLength = 20;
	int* tetrisBoard;
public:
	board(){
		tetrisBoard = new int[200]{};
	};
	bool placePiece(int x, int y, int value);
	int getPiece(int x, int y);
	int getRowSize(int row);
};


#endif //TUTORIAL05_TRIANGLE_BOARD_HPP
