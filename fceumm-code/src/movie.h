#ifndef __FCEU_MOVIE_H_
#define __FCEU_MOVIE_H_

void FCEUMOV_AddJoy(uint8 *);
void FCEUMOV_CheckMovies(void);
void FCEUMOV_Stop(void);
void FCEUMOV_AddCommand(int cmd);
void FCEU_DrawMovies(uint8 *);
int FCEUMOV_IsPlaying(void);

#endif
