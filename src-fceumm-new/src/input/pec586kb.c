#include <string.h>
#include "share.h"
//#include "Pec586kb.h"
//#define AK2(x,y)        ( (FKB_##x) | (FKB_##y <<8) )
//#define AK(x)                 FKB_##x
/*
static uint8 bufit[0x61];
static uint8 ksmode;
static uint8 ksindex;


static uint16 matrix[13][8]=
{
 {AK(4),        AK(G),        AK(F),          AK(C),       AK(F2),     AK(E),        AK(5),         AK(V)     },
 {AK(2),        AK(D),        AK(S),          AK(END),     AK(F1),     AK(W),        AK(3),         AK(X)     },
 {AK(INSERT),   AK(BACK),     AK(NEXT),       AK(RIGHT),   AK(F8),     AK(PRIOR),    AK(DELETE),    AK(HOME)  },
 {AK(9),        AK(I),        AK(L),          AK(COMMA),   AK(F5),     AK(O),        AK(0),         AK(PERIOD)},
 {AK(RBRACKET), AK(RETURN),   AK(UP),         AK(LEFT),    AK(F7),     AK(LBRACKET), AK(BACKSLASH), AK(DOWN)  },
 {AK(Q),        AK(CAPITAL),  AK(Z),          AK(TAB),     AK(ESCAPE), AK(A),        AK(1),         AK(LCONTROL)},
 {AK(7),        AK(Y),        AK(K),          AK(M),       AK(F4),     AK(U),        AK(8),         AK(J)},
 {AK(MINUS),    AK(SEMICOLON),AK(APOSTROPHE), AK(SLASH),   AK(F6),     AK(P),        AK(EQUALS),    AK(LSHIFT)},
 {AK(T),        AK(H),        AK(N),          AK(SPACE),   AK(F3),     AK(R),        AK(6),         AK(B)},
 {0,            0,            0,              0,           0,          0,            0,             0},
 {AK(LMENU),    AK(NUMPAD4),  AK(NUMPAD7),    AK(F11),     AK(F12),    AK(NUMPAD1),  AK(NUMPAD2),   AK(NUMPAD8)},
 {AK(SUBTRACT), AK(ADD),      AK(MULTIPLY),   AK(NUMPAD9), AK(F10),    AK(NUMPAD5),  AK(DIVIDE),    AK(NUMLOCK)},
 {AK(GRAVE),    AK(NUMPAD6),  AK(PAUSE),      AK(SPACE),   AK(F9),     AK(NUMPAD3),  AK(DECIMAL),   AK(NUMPAD0)},
};

static void FP_FASTAPASS(1) Pec586KB_Write(uint8 v)
{
 if((ksmode&2) && !(v&2))
   ksindex=(ksindex+1)%13;
 ksmode=v;
}

static uint8 FP_FASTAPASS(2) Pec586KB_Read(int w, uint8 ret)
{
 if(w)
 {
  int x;

  ret&=~2;
    if(bufit[matrix[ksindex][ksmode&1][x]&0xFF]||bufit[matrix[ksindex][ksmode&1][x]>>8])
      ret|=1<<(x+1);
  ret^=0x1E;
 }
 return(ret);
}

static void Pec586KB_Strobe(void)
{
 ksmode=0;
 ksindex=0;
}

static void FP_FASTAPASS(2) Pec586KB_Update(void *data, int arg)
{
 memcpy(bufit+1,data,0x60);
}

static INPUTCFC Pec586KB={Pec586KB_Read,Pec586KB_Write,Pec586KB_Strobe,Pec586KB_Update,0,0};

INPUTCFC *FCEU_InitPec586KB(void)
{
 memset(bufit,0,sizeof(bufit));
 ksmode=ksindex=0;
 return(&Pec586KB);
}
*/
