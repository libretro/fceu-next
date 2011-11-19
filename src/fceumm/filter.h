int32 NeoFilterSound(int32 *in, int32 *out, uint32 inlen, int32 *leftover);
void MakeFilters(int32 rate);

#if SOUND_QUALITY == 1
void SexyFilter(int32 *in, int32 *out, int32 count);
#endif
