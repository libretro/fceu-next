#ifndef __FCEU_PPUVIEW_H_
#define __FCEU_PPUVIEW_H_

extern int PPUViewScanline;
extern int PPUViewer;
extern int scanline;

void PPUViewDoBlit();
void DoPPUView();
void UpdatePPUView(int refreshchr);

#endif
