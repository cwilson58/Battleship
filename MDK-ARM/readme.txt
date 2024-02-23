Author: Cameron Wilson
Email: caw522@uregina.ca
SID: 200430766

The data is modeled in the way as follows:
No shot attempted: 0
Ship present: 1
Ship hit: 2
Miss: 3
colours for the above should be stored in an array with the index matching the state.
Everything is 8x8 as that is the matrix size.

States for gameplay are as follows
Pre-game: 0
Placement: 1
Normal Gameplay: 2

NOTES:
	LEDS:
	After talking to Robert Martins I was given permission to use a library for my LEDs. This library requires the HAL libraries and was given a green light by robert.