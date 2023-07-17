//
// Created by Sapay on 4/6/2019.
//

#include "board.hpp"

void board(){
}

bool board::placePiece(int x, int y, int value) {
	numOfFaces++;
	board::tetrisBoard[x * board::boardRowLength + y] = value;
	return true;
}

int board::getPiece(int x, int y){
	return board::tetrisBoard[x * board::boardRowLength + y];
}

int board::getRowSize(int row){
	int size = 0;
	for(int i = 0; i < boardRowLength; i++)
		if(board::tetrisBoard[row * boardRowLength + i] != 0)
			size++;
	return size;
}