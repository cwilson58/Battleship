#include "battleship.h"
//putting these in the header gives a linker error, they are the standard colours for everything
struct pixel nothing = {.r = 0,.g=0,.b=0};
struct pixel hit = {.r = 5,.g=0,.b=0};
struct pixel miss = {.r = 5,.g=5,.b=5};
struct pixel ship = {.r = 5,.g=0,.b=5};
struct pixel indicator = {.r = 0,.g=5,.b=0};

//The following array just relates a state number to a colour, it is hardcoded within the battleship_init() function
struct pixel states[4];


/**
* This function does battleship specific initialization
* This does not initialize peripherals, but it does modify GPIOB pins to be inputs
* This also enables the use of the onboard LED for debugging
*/
void battleship_init(){
	//Turn on port C clocks so we can use the user button
	RCC->APB2ENR |=  RCC_APB2ENR_IOPCEN;
	states[0] = nothing;
	states[1] = ship;
	states[2] = hit;
	states[3] = miss;
	//PA5 (on board LED)
	GPIOA->CRL |= GPIO_CRL_MODE5;
	GPIOA->CRL &= ~GPIO_CRL_CNF5;
	GPIOB->CRL |= 0x00880000;
	GPIOB->CRL &= 0xFF88FFFF;
}


/**
*	This initializes the optical switches and the respecitive interrupts
* NOTE: HAL MESSED WITH THE MASKS FOR THIS,PULLED OUT THE EFFECTED ONES INTO HARD CODED VALUES
*/
void OPTICAL_SWITCH_INIT(){
	//ensure the pins are correctly configured to be floating inputs as the library sets them all to be outputs by default
	GPIOB->CRH &= ~GPIO_CRH_MODE9 & ~GPIO_CRH_MODE10 & ~GPIO_CRH_CNF9 & !GPIO_CRH_CNF10;
	GPIOB->CRH |= GPIO_CRH_CNF9_0 | GPIO_CRH_CNF10_0;
	//Enable the PB9 Inturrupt
	AFIO->EXTICR[2] |= AFIO_EXTICR3_EXTI9_PB; //setting the pin source
	EXTI->IMR |= EXTI_IMR_MR9; //unmask the pin
	EXTI->RTSR |= EXTI_RTSR_TR9; //selecting RISING edge
	NVIC->ISER[0] |= ((uint32_t)0x00800000);//NVIC_ISER_SETENA_23; //NVIC nonsense (Check vector table for bit pos)
	//Enable the PB10 IRQ
	AFIO->EXTICR[2] |= AFIO_EXTICR3_EXTI10_PB; //setting pin souce
	EXTI->IMR |= EXTI_IMR_MR10;
	EXTI->RTSR |= EXTI_RTSR_TR10;
	NVIC->ISER[1] |= ((uint32_t)0x00000100);// NVIC_ISER_SETENA_8; //position 40
}

/**
*	This handles player twos turn switching, firing as well as ship placement
* NOTE: these are reversed from the visual X - Y 
*/
void EXTI9_5_IRQHandler(void){
	EXTI->PR |= EXTI_PR_PR9; //clear

	if(turn == 1){
		if(p1Board[currPositionY][currPositionX] == SHIPSTATE){
			p1Board[currPositionY][currPositionX] = HITSTATE;
			p2Hits++;
		}
		else if(p1Board[currPositionY][currPositionX] == NOTHINGSTATE)
			p1Board[currPositionY][currPositionX] = MISSSTATE;
		//change turn
		turn = 0;
	}
	//if we are in placement and turn is odd
	else if(turn > 2 && (turn % 2) == 1){
		shipPlaced = 0;
		turn++;
	}


}

/**
*	This handles player ones's turn switching, firing as well as ship placement
* NOTE: these are reversed from the visual X - Y 
*/
void EXTI15_10_IRQHandler(void){
	EXTI->PR |= EXTI_PR_PR10; //clear
	if(turn == 0){
		if(p2Board[currPositionY][currPositionX] == SHIPSTATE){
			p2Board[currPositionY][currPositionX] = HITSTATE;
			p1Hits++;
		}
		else if(p2Board[currPositionY][currPositionX] == NOTHINGSTATE){
			p2Board[currPositionY][currPositionX] = MISSSTATE;
		}
		//change turn
		turn = 1;
	}
	else if(turn == -1){
		gameState = SHIPPLACEMENT;
		turn = 0;
	}
	else if(turn >= 2 && turn%2 == 0){ //if we are in placement and on an even turn value
		shipPlaced = 0;
		turn++;
	}
}

/**
*	Tim1 channel one, used for the buzzer
*/
void tim1GpioSetup(void){
	RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
	RCC->APB2ENR |=  RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN;
	GPIOA->CRH |= GPIO_CRH_CNF8 | GPIO_CRH_MODE8_1 |GPIO_CRH_MODE8; 
	//The following code was taken from daves lab notes I sure hope it works!
	TIM1->CR1 |= TIM_CR1_CEN; // Enable Timer1
	TIM1->CR2 |= TIM_CR2_OIS1; // Output Idle State for Channel 1 OC1=1 when MOE=0
	TIM1->EGR |= TIM_EGR_UG; // Reinitialize the counter
	TIM1->CCMR1 |= TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1; // PWM mode 1,
	TIM1->CCMR1 |= TIM_CCMR1_OC1PE | TIM_CCMR1_OC1FE; // Preload Enable, Fast Enable
	TIM1->CCER |= TIM_CCER_CC1E; //Enable CH1
	TIM1->PSC = 0x095F; // Divide 24 MHz by 2400 (PSC+1), PSC_CLK= 10000 Hz, 1 count = 0.1 ms
	TIM1->ARR = 200; // 100 counts = 10 ms or 100 Hz
	TIM1->CCR1 = 50; // 50 counts = 5 ms = 50% duty cycle

}

/**
* Helper function that will play a number of short buzzer tones
*/
void buzzerPlay(int numberOfShorts){
	for(int i = 0; i < numberOfShorts; i++){
		TIM1->BDTR |= TIM_BDTR_MOE | TIM_BDTR_OSSI; // Main Output Enable, Force Idle Level First
		TIM1->CR1 |= TIM_CR1_ARPE | TIM_CR1_CEN; // Enable Timer1
		HAL_Delay(1000);
		TIM1->BDTR &= ~(TIM_BDTR_MOE | TIM_BDTR_OSSI); // Main Output Enable, Force Idle Level First
		TIM1->CR1 &= ~(TIM_CR1_ARPE | TIM_CR1_CEN); // Enable Timer1
		HAL_Delay(1000);
	}
}

/**
* This initializes the ADC for use in battleship, we need 4 channels total that are on PA0, PA1, PA4 and PA6
*/
void ADC_INIT(){
		// Enable Clock
		RCC->APB2ENR |=  RCC_APB2ENR_IOPCEN;
	// Enable IO on A
		RCC->APB2ENR |=  RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN | RCC_APB2ENR_ADC1EN;
	// Set PA0,1,4,6 to the desired setting
	//GPIOA->CRL  &= 0xFFFFFFF0;
	GPIOA->CRL  &= 0xF0F0FF00;
	//Turn on the ADC
		ADC1->CR2 |= 0x00000001;
}

/**
* Helper function that will read an ADC channel and return a value based on the battleship grid
* this deals with the raw data so main does not have to
*/
uint16_t READ_ADC(int channel){
	//0,1,4,6 are the possible channels
	if(channel == 0){
		ADC1->SQR3 = 0x00000000; //WRITE THE CHANNEL NUMBER HERE BITS 29:25. 0,1,4,6
	}
	else if(channel == 1){
		ADC1->SQR3 = 1; //WRITE THE CHANNEL NUMBER HERE BITS 29:25. 0,1,4,6
	}
	else if(channel == 4){
		ADC1->SQR3 = 4; //Write a 4 to the needed bits
	}
	else if(channel == 6){
		ADC1->SQR3 = 6; //Write a 6 (4 + 2) to the needed bits
	}
	else{
		return -1; //invalid channel passed in
	}
	//start
	ADC1->CR2 = 0x00000001;
	//we are stuck in conversion world
  while((ADC1->SR & 0x00000002) != 2){
		//Just wait for the loop to break
	}
	//Translate Raw data.
	int rawData = ADC1->DR & 0x0000FFFF;
	if(rawData <= 0x2FF){ //This needed an increased range due to noise in the circuits
		return 0;
	}
	else if(rawData <= 0x3FF){
		return 1;
	}
	else if(rawData <= 0x5FF){
		return 2;
	}
	else if(rawData <= 0x7FF){
		return 3;
	}
	else if(rawData <= 0x9FF){
		return 4;
	}
	else if(rawData <= 0xBFF){
		return 5;
	}
	else if(rawData <= 0xDFF){
		return 6;
	}
	else{
		return 7;
	}
}

//this gives an example battleship board
void example_battleship_board(struct pixel *framebuffer){ 
	//3 hits top left, 5 misses, 8 misses, 4 ship on the LEFT, rest not hit

	for(int i = 0; i < 3; i++){
		framebuffer[i] = hit;
	}
	for(int i = 3; i < 16; i++){
		framebuffer[i] = miss;
	}
	framebuffer[16] = ship;
	framebuffer[17] = ship;
	framebuffer[18] = ship;
	framebuffer[19] = ship;
	//shit vertically in the bottom right
	framebuffer[63-1] = ship;
	framebuffer[63-9] = ship;
	framebuffer[63-17] = ship;
	framebuffer[63-17-8] = ship;
	framebuffer[63-17-16] = ship;
}

/**
* Helper function that will translate gamestate into a visual representation
* this interfaces with the libraries framebuffer (the pixel param)
* anywhere that you see 64+ means it is going to the second matrix in the chain
*/
void translate_gamestate_to_leds(struct pixel *framebuffer,int bottom[][8],int top[][8]){
	//buffers go from bottom to top
	for(int i = 0; i < 8; i++){
		//if(i % 2 == 0){
			for(int j = 0; j < 8; j++){
				framebuffer[(i*8)+j] = states[bottom[i][j]]; //we store the state in the board coresponding to its index
				if(top[i][j] != SHIPSTATE){
					framebuffer[64+(i*8)+j] = states[top[i][j]]; //if we gp 64 ahead we hit the next board and thus only have to loop once
				}
				//the top should not show any ships it should show those as nothing.
				else{
					framebuffer[64+(i*8)+j] = states[NOTHINGSTATE];
				}
				
			}
	}
}
