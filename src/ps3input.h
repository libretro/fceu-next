/******************************************************************************* 
 * ps3input.h - FCEUNext PS3
 *
 *  Created on: Aug 18, 2011
********************************************************************************/

#ifndef PS3INPUT_H_
#define PS3INPUT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>

/* Input bitmasks */
#define BTN_A				1 << 0
#define BTN_B				1 << 1
#define BTN_SELECT			1 << 2
#define BTN_START			1 << 3
#define BTN_UP				1 << 4
#define BTN_DOWN			1 << 5
#define BTN_LEFT			1 << 6
#define BTN_RIGHT			1 << 7

#define BTN_LASTGAMEBUTTON BTN_RIGHT

#define BTN_QUICKSAVE			1 << 8
#define BTN_QUICKLOAD			1 << 9
#define BTN_INCREMENTSAVE		1 << 10
#define BTN_DECREMENTSAVE		1 << 11
#define BTN_INCREMENTCHEAT		1 << 12
#define BTN_DECREMENTCHEAT		1 << 13
#define BTN_EXITTOMENU			1 << 14
#define BTN_CHEATENABLE			1 << 15
#define BTN_CHEATDISABLE		1 << 16
#define BTN_LOAD_MOVIE			1 << 17
#define BTN_SOFTRESET			1 << 18
#define BTN_RESET			1 << 19
#define BTN_PAUSE			1 << 20
#define BTN_FASTFORWARD			1 << 21
#define BTN_INCREMENTTURBO		1 << 22
#define BTN_DECREMENTTURBO		1 << 23
#define BTN_NONE			1 << 24
#define BTN_TOGGLEPPU			1 << 25
#define BTN_INCREMENT_PALETTE		1 << 26
#define BTN_DECREMENT_PALETTE		1 << 27
#define BTN_INGAME_MENU			1 << 28

#define BTN_FIRSTEXTRABUTTON BTN_QUICKSAVE

#define CONTROL_SCHEME_DEFAULT		0
#define CONTROL_SCHEME_NEW		1
#define CONTROL_SCHEME_CUSTOM		2

#define MAX_PADS 7

typedef struct{
	uint32_t		ButtonCircle[MAX_PADS];
	uint32_t		ButtonCross[MAX_PADS];
	uint32_t		ButtonTriangle[MAX_PADS];
	uint32_t		ButtonSquare[MAX_PADS];
	uint32_t		ButtonSelect[MAX_PADS];
	uint32_t		ButtonStart[MAX_PADS];
	uint32_t		ButtonL1[MAX_PADS];
	uint32_t		ButtonR1[MAX_PADS];
	uint32_t		ButtonL2[MAX_PADS];
	uint32_t		ButtonR2[MAX_PADS];
	uint32_t		DPad_Up[MAX_PADS];
	uint32_t		DPad_Down[MAX_PADS];
	uint32_t		DPad_Left[MAX_PADS];
	uint32_t		DPad_Right[MAX_PADS];
	uint32_t		ButtonR2_ButtonR3[MAX_PADS];
	uint32_t		ButtonL2_ButtonR3[MAX_PADS];
	uint32_t		ButtonL2_ButtonR2[MAX_PADS];
	uint32_t		AnalogR_Right[MAX_PADS];
	uint32_t		AnalogR_Left[MAX_PADS];
	uint32_t		AnalogR_Up[MAX_PADS];
	uint32_t		AnalogR_Down[MAX_PADS];
	uint32_t		ButtonL2_AnalogR_Right[MAX_PADS];
	uint32_t		ButtonL2_AnalogR_Left[MAX_PADS];
	uint32_t		ButtonL2_AnalogR_Up[MAX_PADS];
	uint32_t		ButtonL2_AnalogR_Down[MAX_PADS];
	uint32_t		ButtonR2_AnalogR_Right[MAX_PADS];
	uint32_t		ButtonR2_AnalogR_Left[MAX_PADS];
	uint32_t		ButtonR2_AnalogR_Up[MAX_PADS];
	uint32_t		ButtonL2_ButtonR2_AnalogR_Down[MAX_PADS];
	uint32_t		ButtonR2_AnalogR_Down[MAX_PADS];
	uint32_t		ButtonL2_ButtonL3[MAX_PADS];
	uint32_t		ButtonR3_ButtonL3[MAX_PADS];
	uint32_t		ButtonL3[MAX_PADS];
	uint32_t		ButtonR3[MAX_PADS];
	uint32_t		AnalogR_Up_Type[MAX_PADS];
	uint32_t		AnalogR_Down_Type[MAX_PADS];
	uint32_t		AnalogR_Left_Type[MAX_PADS];
	uint32_t		AnalogR_Right_Type[MAX_PADS];
	uint32_t		AnalogL_Left[MAX_PADS];
	uint32_t		AnalogL_Right[MAX_PADS];
	uint32_t		AnalogL_Up[MAX_PADS];
	uint32_t		AnalogL_Down[MAX_PADS];
} PS3InputList;

#define Input_MapButton(buttonmap, next, defaultbutton) \
	if(defaultbutton == NULL) \
		buttonmap = Input_GetAdjacentButtonmap(buttonmap, next); \
	else \
		buttonmap = defaultbutton;

extern PS3InputList PS3Input;

const char * Input_PrintMappedButton(uint32_t mappedbutton);
int Input_GetAdjacentButtonmap(uint32_t buttonmap, uint32_t next);

#ifdef __cplusplus
}
#endif

#endif
