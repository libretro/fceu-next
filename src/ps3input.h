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
#define BTN_A				1
#define BTN_B				2
#define BTN_SELECT			4
#define BTN_START			8
#define BTN_UP				16
#define BTN_DOWN			32
#define BTN_LEFT			64
#define BTN_RIGHT			128

#define BTN_LASTGAMEBUTTON BTN_RIGHT

#define BTN_QUICKSAVE			256
#define BTN_QUICKLOAD			512
#define BTN_INCREMENTSAVE		1024
#define BTN_DECREMENTSAVE		2048
#define BTN_INCREMENTCHEAT		4096
#define BTN_DECREMENTCHEAT		8192
#define BTN_EXITTOMENU			16384
#define BTN_CHEATENABLE			32768
#define BTN_CHEATDISABLE		65536
#define BTN_SOFTRESET			131072
#define BTN_RESET			262144
#define BTN_PAUSE			524288
#define BTN_FASTFORWARD			1048576
#define BTN_NONE			2097152
#define BTN_TOGGLEPPU			4194304
#define BTN_INCREMENT_PALETTE		8388608
#define BTN_DECREMENT_PALETTE		16777216
#define BTN_INGAME_MENU			33554432

#define BTN_FIRSTEXTRABUTTON BTN_QUICKSAVE

#define CONTROL_SCHEME_DEFAULT		0
#define CONTROL_SCHEME_CUSTOM		1

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
