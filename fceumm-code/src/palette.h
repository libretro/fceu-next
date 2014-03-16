#ifndef __FCEU_PALETTE_H_
#define __FCEU_PALETTE_H_

typedef struct {
	uint8 r, g, b;
} pal;

extern pal *palo;
void FCEU_ResetPalette(void);

void FCEU_ResetPalette(void);
void FCEU_ResetMessages();
void FCEU_LoadGamePalette(void);
void FCEU_DrawNTSCControlBars(uint8 *XBuf);

#endif
