#ifndef __FCEU_PALETTE_H
#define __FCEU_PALETTE_H

typedef struct {
	uint8 r,g,b;
} pal;

extern pal *palo;
void FCEU_ResetPalette(void);

void FCEU_ResetPalette(void);
void FCEU_ResetMessages();
void FCEU_LoadGamePalette(void);

#endif
