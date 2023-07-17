# Tetris created from using "Tutorial 5 - Creating a triangle" as a starting point

### Features added:
 - screen refresh so game updates are visible
 - custom touch inputs to play the game
 - removal of completed rows, however no visual for score tracking
 - spawn a random piece after last becomes part of the board
 - simplistic collision system for piece-board interaction

## Screenshot

<div align="center" >
  <img align="center" src="./Tutorial_5_Screenshot.png" height="400px"> 
  <span text-align="center">&nbsp;&nbsp;&nbsp;&nbsp;turned into&nbsp;&nbsp;&nbsp;&nbsp;</span> 
  <img align="center" src="./TetrisBoard.png" height="400px">
</div>

## Controls

<div align="center" >
  <div text-align="center"> Piece Translation - swipe in one of the three directions before taking your finger off the screen (touch release): </div>
  <img align="center" src="./touch - translate left.png" height="400px"> 
  <img align="center" src="./touch - translate down.png" height="400px">
  <img align="center" src="./touch - translate right.png" height="400px">
</div>

# 

<div align="center" >
  <div text-align="center"> Piece Rotation: </div>
  <img align="center" src="./touch - clockwise.png" height="400px"> 
  <img align="center" src="./touch - counter-clockwise.png" height="400px">
</div>

<div align="center" >
  <div text-align="center">A full circle is not required to be drawn to rotate a piece, instead the piece is rotated after each quarter circle is drawn as it's faster for the player to input. If the player ever makes a mistake they can quickly update the rotation by drawing in the opposite rotation as seen below:</div>
  <img align="center" src="./touch - when rotation commands are processed.png" height="400px"> 
  <div text-align="center">The input above will result in the rotation before the input, however it will rotate once clockwise then wait for touch release or another rotation input before applying the last pending (counter-clockwise) rotation.</div>
</div>
