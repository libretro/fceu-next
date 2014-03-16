#ifndef __FCEU_FDS_H_
#define __FCEU_FDS_H_

void FDSSoundReset(void);

void FCEU_FDSInsert(int oride);
void FCEU_FDSEject(void);
void FCEU_FDSSelect(void);

extern uint32 lastDiskPtrRead, lastDiskPtrWrite;

#endif
