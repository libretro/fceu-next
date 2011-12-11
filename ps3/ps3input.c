/******************************************************************************* 
 * ps3input.c - FCEUNext PS3
 *
 *  Created on: Aug 18, 2011
********************************************************************************/

#include "ps3input.h"

const char * Input_PrintMappedButton(uint32_t mappedbutton)
{
	switch(mappedbutton)
	{
		case BTN_A:
			return "Button A";
		case BTN_B:
			return "Button B";
		case BTN_SELECT:
			return "Button Select";
		case BTN_START:
			return "Button Start";
		case BTN_LEFT:
			return "D-Pad Left";
		case BTN_RIGHT:
			return "D-Pad Right";
		case BTN_UP:
			return "D-Pad Up";
		case BTN_DOWN:
			return "D-Pad Down";
		case BTN_QUICKSAVE:
			return "Save State";
		case BTN_QUICKLOAD:
			return "Load State";
		case BTN_INCREMENTSAVE:
			return "Increment state position";
		case BTN_DECREMENTSAVE:
			return "Decrement state position";
		case BTN_INCREMENTCHEAT:
			return "Increment cheat position";
		case BTN_DECREMENTCHEAT:
			return "Decrement cheat position";
		case BTN_CHEATENABLE:
			return "Enable selected cheat";
		case BTN_CHEATDISABLE:
			return "Disable selected cheat";
		case BTN_EXITTOMENU:
			return "Exit to menu";
		case BTN_TOGGLEPPU:
			return "Toggle PPU Mode";
		case BTN_NONE:
			return "None";
		case BTN_INCREMENT_PALETTE:
			return "Next Palette";
		case BTN_DECREMENT_PALETTE:
			return "Previous Palette";
		case BTN_FASTFORWARD:
			return "Fast forward";
		case BTN_INGAME_MENU:
			return "Ingame Menu";
		default:
			return "Unknown";
	}
}

//bool next: true is next, false is previous
int Input_GetAdjacentButtonmap(uint32_t buttonmap, uint32_t next)
{
	switch(buttonmap)
	{
		case BTN_UP:
			return next ? BTN_DOWN : BTN_NONE;
		case BTN_DOWN:
			return next ? BTN_LEFT : BTN_UP;
		case BTN_LEFT:
			return next ? BTN_RIGHT : BTN_DOWN;
		case BTN_RIGHT:
			return next ?  BTN_A : BTN_LEFT;
		case BTN_A:
			return next ? BTN_B : BTN_RIGHT;
		case BTN_B:
			return next ? BTN_SELECT : BTN_A;
		case BTN_SELECT:
			return next ? BTN_START : BTN_B;
		case BTN_START:
			return next ? BTN_QUICKSAVE : BTN_SELECT;
		case BTN_QUICKSAVE:
			return next ? BTN_QUICKLOAD : BTN_START;
		case BTN_QUICKLOAD:
			return next ? BTN_INCREMENTCHEAT : BTN_QUICKSAVE;
		case BTN_INCREMENTCHEAT:
			return next ? BTN_DECREMENTCHEAT : BTN_QUICKLOAD;
		case BTN_DECREMENTCHEAT:
			return next ? BTN_FASTFORWARD : BTN_INCREMENTCHEAT;
		case BTN_FASTFORWARD:
			return next ? BTN_EXITTOMENU : BTN_DECREMENTCHEAT;
		case BTN_EXITTOMENU:
			return next ? BTN_CHEATENABLE : BTN_FASTFORWARD;
		case BTN_CHEATENABLE:
			return next ? BTN_CHEATDISABLE : BTN_EXITTOMENU;
		case BTN_CHEATDISABLE:
			return next ? BTN_DECREMENTSAVE : BTN_CHEATENABLE;
		case BTN_DECREMENTSAVE:
			return next ? BTN_INCREMENTSAVE : BTN_CHEATDISABLE;
		case BTN_INCREMENTSAVE:
			return next ? BTN_TOGGLEPPU : BTN_DECREMENTSAVE;
		case BTN_TOGGLEPPU:
			return next ? BTN_DECREMENT_PALETTE : BTN_INCREMENTSAVE;
		case BTN_DECREMENT_PALETTE:
			return next ? BTN_INCREMENT_PALETTE : BTN_TOGGLEPPU;
		case BTN_INCREMENT_PALETTE:
			return next ? BTN_INGAME_MENU  : BTN_DECREMENT_PALETTE;
		case BTN_INGAME_MENU:
			return next ? BTN_NONE : BTN_INCREMENT_PALETTE;
		case BTN_NONE:
			return next ? BTN_UP : BTN_INGAME_MENU;
		default:
			return BTN_NONE;
	}
}
