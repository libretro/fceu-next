#ifdef __cplusplus
extern "C"{
void net_init();
void net_shutdown();

void net_send(const char *text,...);
}
#endif
