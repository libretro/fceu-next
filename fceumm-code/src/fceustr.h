#ifndef __FCEUSTR_H_
#define __FCEUSTR_H_

typedef struct {
	uint8 *data;
	uint32 len;  /* Not including extra NULL character. */
} fceustr;

#endif
