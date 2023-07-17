// Copyright 2016 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <android/log.h>
#include <android_native_app_glue.h>
#include <android/sensor.h>
#include "VulkanMain.hpp"
#include <jni.h>

// Original Code provided by

// Added headers
#include "simpleVectorMath.h"
#include <chrono>
#include <queue>
#include <unistd.h>

//TODO write code to handle gestures and call the appropriate commands in the game code - Almost Done

//added variables
double startXCoord, startYCoord, lastXCoord = -1, lastYCoord = -1, currentXCoord, currentYCoord, distX, distY, vecAngle, refreshRate;
int windowHeight = 0, windowWidth = 0;
bool vulkanInitialized;
vec2 *prevVec, startVec, positiveX(1,0), positiveY(0,1);
// startTime is used for the touch interaction
std::chrono::system_clock::time_point startTime, gameStartTime;
// used to see if it's time to call a movement update for the current piece
std::chrono::milliseconds gameCurrentLapsedTime, gameLastLapsedTime;
//should never get above 2 in size
std::queue<vec2> touchEventQueue;

// Process the next main command.
// TODO - add to the handle commmands
void handle_cmd(android_app* app, int32_t cmd) {
	switch (cmd) {
    	case APP_CMD_INIT_WINDOW:
      	// The window is being shown, get it ready.
      	InitVulkan(app);
      	break;
    	case APP_CMD_TERM_WINDOW:
      	// The window is being hidden or closed, clean it up.
      	DeleteVulkan();
      	break;
    	default:
      	__android_log_print(ANDROID_LOG_INFO, "Vulkan Tutorials", "event not handled: %d", cmd);
  	}
}

// added function
void touchscreenCommands(vec2 first){
	// move the piece in the positive or negative x direction and move the piece in the positive y direction (down)

	// commented out as it's only used for testing purposes
	//__android_log_print(ANDROID_LOG_INFO, "Vulkan Tutorials", "command X angle: %f", angle(first, positiveX));
	//__android_log_print(ANDROID_LOG_INFO, "Vulkan Tutorials", "command Y angle: %f", angle(first, positiveY));

	// .707107 is cos(45), and .766044 is cos(40). The conditionals below are to make it easier to read the touch commands
	// greater than cos(45) is equal to a angle less than 45, less than absolute cos(40) means the angles between 40 and 140
	// will satisfy the condition as we are really trying to test for if the user is moving their finger down. (the screen of the pixel 2 XL
	// isn't really accurate when reading the Y axis
	if(angle(first, positiveX) > .707107 && absAngle(first,positiveY) < .766044){
		__android_log_print(ANDROID_LOG_INFO, "Touch interface", " command translate right");
		translate(first.x/5,0);
	}else if(angle(first, positiveX) < -.707107 && absAngle(first,positiveY) < .766044){
		__android_log_print(ANDROID_LOG_INFO, "Touch interface", " command translate left");
		translate(first.x/5,0);
	} else if(angle(first,positiveY) > .707107 && absAngle(first,positiveX) < .707107){
		__android_log_print(ANDROID_LOG_INFO, "Touch interface", " command translate down");
		translate(0,first.y/5);
	}
}

// added function
void touchscreenCommands(vec2 first, vec2 second){
	//this takes care of the rotation commands
	//__android_log_print(ANDROID_LOG_INFO, "Touch interface", " rotation commands processed");
	if(angle(first, positiveX) > .707107){
		if(angle(second,positiveY) > .707107){
			// clockwise rotation
			__android_log_print(ANDROID_LOG_INFO, "Touch interface", " clockwise rotation commands processed");
			clockwiseRotation();
		} else if(angle(second,positiveY) < -.707107){
			// counter clockwise rotation
			__android_log_print(ANDROID_LOG_INFO, "Touch interface", " counter clockwise rotation commands processed");
			cClockwiseRotation();
		}
	} else if (angle(first, positiveX) < -.707107){
		if(angle(second,positiveY) > .707107){
			// counter clockwise rotation
			__android_log_print(ANDROID_LOG_INFO, "Touch interface", " counter clockwise rotation commands processed");
			cClockwiseRotation();
		} else if(angle(second,positiveY) < -.707107){
			// clockwise rotation
			__android_log_print(ANDROID_LOG_INFO, "Touch interface", " clockwise rotation commands processed");
			clockwiseRotation();
		}
	} else if(angle(first,positiveY) > .707107){
		if(angle(second,positiveX) > .707107){
			// counter clockwise rotation
			__android_log_print(ANDROID_LOG_INFO, "Touch interface", " counter clockwise rotation commands processed");
			cClockwiseRotation();
		} else if(angle(second,positiveX) < -.707107){
			// clockwise rotation
			__android_log_print(ANDROID_LOG_INFO, "Touch interface", " clockwise rotation commands processed");
			clockwiseRotation();
		}
	} else if(angle(second,positiveY) < -.707107){
		if(angle(second,positiveX) > .707107){
			// clockwise rotation
			__android_log_print(ANDROID_LOG_INFO, "Touch interface", " clockwise rotation commands processed");
			clockwiseRotation();
		} else if(angle(second,positiveX) < -.707107){
			// counter clockwise rotation
			__android_log_print(ANDROID_LOG_INFO, "Touch interface", " counter clockwise rotation commands processed");
			cClockwiseRotation();
		}
	}

}

// added function
// basic structure of the function below was found at
// stackoverflow.com/questions/21721589/how-do-i-handle-touch-screen-events-in-android-native-code-without-going-through
// the logic was written from scratch, possible fix is to not poll the current coordinates as fast as they are currently
int32_t touchEventHandler(android_app *app, AInputEvent *event){
	if(windowHeight == 0){
		windowHeight = ANativeWindow_getHeight(app->window);
		windowWidth = ANativeWindow_getWidth(app->window);
	}
	int32_t eventType = AInputEvent_getType(event);
	switch(eventType){
		case AINPUT_EVENT_TYPE_MOTION:
			switch(AInputEvent_getSource(event)){
				case AINPUT_SOURCE_TOUCHSCREEN:
					int action = AKeyEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
					size_t pointerIndex = AMotionEvent_getPointerId(event,AMOTION_EVENT_ACTION_POINTER_INDEX_MASK);
					switch(action){
						case AMOTION_EVENT_ACTION_DOWN:
							startTime = std::chrono::system_clock::now();
							break;

						case AMOTION_EVENT_ACTION_UP:
							if(touchEventQueue.size() == 1){
								touchscreenCommands(touchEventQueue.front());
								touchEventQueue.pop();
							} else if(touchEventQueue.size() == 2){
								touchscreenCommands(touchEventQueue.front(),touchEventQueue.back());
								touchEventQueue.pop();
								touchEventQueue.pop();
							}
							lastXCoord = -1;
							lastYCoord = -1;
							//startVec = nullptr;
							break;

						case AMOTION_EVENT_ACTION_MOVE:
							if(lastXCoord == -1 || lastYCoord == -1){
								lastXCoord = AMotionEvent_getX(event, pointerIndex);
								lastYCoord = AMotionEvent_getY(event, pointerIndex);
								startXCoord = lastXCoord;
								startYCoord = lastYCoord;
								startVec = vec2(0,0);
								//__android_log_print(ANDROID_LOG_INFO, "Touch interface", " command startVec initialize");
							} else {
								currentXCoord = AMotionEvent_getX(event, pointerIndex);
								currentYCoord = AMotionEvent_getY(event, pointerIndex);
								distX = currentXCoord - lastXCoord;
								distY = currentYCoord - lastYCoord;
								vec2 currentVec(distX, distY);
								/*if(startVec == nullptr){
									*startVec = vec2(distX, distY);
								}*/
								if(prevVec != nullptr){
									if((distX / windowWidth < .181 && distY / windowHeight < .09)
									&& std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - startTime) > std::chrono::milliseconds(200)){
										//this is treated as no movement, this will flush the vector buffer and execute all commands present
										//however it needs happen for at least 200 milliseconds, don't update last Coords
										// dump vector queues
										// send the vector(s) to a function to decide what command the user wants to do
										if(touchEventQueue.size() == 1){
											touchscreenCommands(touchEventQueue.front());
											touchEventQueue.pop();
										} else if(touchEventQueue.size() == 2){
											touchscreenCommands(touchEventQueue.front(),touchEventQueue.back());
											touchEventQueue.pop();
											touchEventQueue.pop();
											lastXCoord = currentXCoord;
											lastYCoord = currentYCoord;
										}
									} else {
										// add vector to the queue if null, else check the angle with the previous vector if they are similar
										// enough then add them together else add to the queue if there is space else pop the first vector and read
										// the second vector to get which touchscreen command it corresponds to
										if(touchEventQueue.size() != 0){
											vecAngle = angle(touchEventQueue.front(),currentVec);
										} else {
											vecAngle = angle(startVec, currentVec);
										}
										//TODO rewrite this section - Done? good enough
										if(vecAngle >= cos(45)) {
											// don't push vector into the queue or update the last Coords
											//__android_log_print(ANDROID_LOG_INFO, "Touch interface", " command similar angle");
										} else if(abs(vecAngle) < cos(45)){
											// if the angle is closer to being orthogonal
											//__android_log_print(ANDROID_LOG_INFO, "Touch interface", " command orthogonal angle");
											if(touchEventQueue.size() == 0){
												touchEventQueue.push(vec2(currentXCoord - startXCoord, currentYCoord - startYCoord));
												lastXCoord = -1;
												lastYCoord = -1;
												//startVec = nullptr;
											} else if (touchEventQueue.size() == 1){
												// push vector with coords current - start
												vec2 tempVec = vec2(currentXCoord - startXCoord, currentYCoord - startYCoord);
												double tempAngle = angle(tempVec,touchEventQueue.back());
												if(tempAngle < cos(45)){
													touchEventQueue.push(tempVec);
												} else {
													touchEventQueue.pop();
													touchEventQueue.push(tempVec);
												}
												lastXCoord = -1;
												lastYCoord = -1;
												//startVec = nullptr;
												startTime = std::chrono::system_clock::now();
												break;
											} else {
												touchscreenCommands(touchEventQueue.front(),touchEventQueue.back());
												touchEventQueue.pop();
											}
										// switch this check with the check for orthogonality, as an angle pass 90 degrees will be closer to the starting point then the previous vector
										} else if(vecAngle < cos(135) || dist(currentVec,startVec) < dist(*prevVec,startVec)){
											// send the vector(s) to a function to decide what command the user wants to do
											//__android_log_print(ANDROID_LOG_INFO, "Touch interface", " command opposite angle");
											if(touchEventQueue.size() == 1){
												touchscreenCommands(*prevVec);
												touchEventQueue.pop();
												lastXCoord = currentXCoord;
												lastYCoord = currentYCoord;
											} else if(touchEventQueue.size() == 2){
												touchEventQueue.pop();
												touchscreenCommands(*prevVec,touchEventQueue.front());
												touchEventQueue.pop();
											}
										}
										startTime = std::chrono::system_clock::now();
									}
								}
								prevVec = &currentVec;
								lastXCoord = currentXCoord;
								lastYCoord = currentYCoord;
							}
							// was used to get the pointer location to better understand how the ui worked
							//__android_log_print(ANDROID_LOG_INFO, "Touch interface", "X location: %f", AMotionEvent_getX(event, pointerIndex));
							//__android_log_print(ANDROID_LOG_INFO, "Touch interface", "Y location: %f", AMotionEvent_getY(event, pointerIndex));
							break;
					}
			}
			//__android_log_print(ANDROID_LOG_INFO, "Touch interface", "drag: %d", eventType);
			break;
	}
	return 0;
}

void android_main(struct android_app* app) {
  	// Set the callback to process system events
  	app->onAppCmd = handle_cmd;
  	app->onInputEvent = touchEventHandler;
  	refreshRate = 60;
	vulkanInitialized = false;
	gameStartTime = std::chrono::system_clock::now();
	gameLastLapsedTime = std::chrono::milliseconds(0);

  	// Used to poll the events in the main loop
  	int events;
  	android_poll_source* source;
  	// need to call the function to initialize the game
  	genNextPiece();
  	// Main loop
  	do {
    	if (ALooper_pollAll(IsVulkanReady() ? 1 : 0, nullptr, &events, (void**)&source) >= 0) {
      		if (source != NULL)
      			source->process(app, source);
    	}

    	// added, this is the code that causes the current piece to move down 1 block every second
		gameCurrentLapsedTime = std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() - gameStartTime));
    	if(vulkanInitialized && gameCurrentLapsedTime  >= std::chrono::seconds(1)){
			gameStartTime = std::chrono::system_clock::now();
			translate(0,1);
    	}

    	// render if vulkan is ready
    	if (IsVulkanReady()) {
    		VulkanDrawFrame();
    		// used to lock the refresh rate to 60 fps as to not burn the phones battery up or cause
    		// the phone to become uncomfortable to hold as it would be too hot. usleep is in microseconds
			usleep(1000000/refreshRate);
    		vulkanInitialized = true;
    	} else {
			usleep(1000);
    	}

  	} while (!app->destroyRequested);
}
