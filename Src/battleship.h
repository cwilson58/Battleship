/*
* Author: Cameron Wilson
* Email: caw522@ureinga.ca
* Modification Date: April 04, 2023
**************************************
* This fule contains all of the macros and headers for this project.
* Please note the following compiler settings were used:
* Optimization: -01
* C version: gnu99
* Arm Compiler default version 6
*/
#pragma once
//If we use the non HAL library, it will not compile as the two conflict. They have similar if not identical macros for what I needed.
#include "stm32f1xx_hal.h" 

/**
* The following macros are for readability sake, to reduct the amount of magic numbers in the code
*/
//play surface states
#define NOTHINGSTATE	0
#define SHIPSTATE			1
#define HITSTATE			2
#define MISSSTATE			3

//game states
#define PREGAME					0
#define SHIPPLACEMENT		1
#define PLAYING					2

/**
* The following externally defined variables are modified within some functions in this header
* Note an inturrupt is used to modify game state, as it provided a smoother experience for use than polling
*/
extern int currPositionX;
extern int currPositionY;
extern int p1Board[8][8];
extern int p2Board[8][8];
extern int turn;
extern int p1Hits;
extern int p2Hits;
extern int gameState;
extern int currentShipLength;
extern int shipPlaced;

/**
* This was orginally defined by the library
* All this does is hold the RGB value for an LED but it makes it easier to interface with the library functions
*/
struct pixel {
    uint8_t g;
    uint8_t r;
    uint8_t b;
};

//The remainder of this file is battleship function definitions
void battleship_init();

void OPTICAL_SWITCH_INIT();
void EXTI9_5_IRQHandler(void);
void EXTI15_10_IRQHandler(void);

void tim1GpioSetup(void);

void buzzerPlay(int numberOfShorts);
void ADC_INIT();
uint16_t READ_ADC(int channel);
//This function runs while we are in idle state
void example_battleship_board(struct pixel *framebuffer);

void translate_gamestate_to_leds(struct pixel *framebuffer,int bottom[][8], int top[][8]);

