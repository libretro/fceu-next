#include "config.h"

#include "logger.h"
#include <netex/net.h>
#include <cell/sysmodule.h>
#include <netex/libnetctl.h>
#include <stdio.h>
#include <sys/timer.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#define BUFSIZE (64 * 1024)
#define TCPDUMP_FILE (SYS_APP_HOME "/tcpdump.dump")
#define TCPDUMP_STACKSIZE (16 * 1024)
#define TCPDUMP_PRIO (2048)

//static unsigned char g_log_buf[BUFSIZE];
//static int g_log_id;
static int g_sid;
static int sock;
static sockaddr_in target;
static char sendbuf[4096];

int if_up_with(int index)
{
      int timeout_count = 10;
      int state;
      int ret;

      (void)index;
      ret = cellNetCtlInit();
      if (ret < 0)
      {
         printf("cellNetCtlInit() failed(%x)\n", ret);
         return (-1);
      }

      for (;;)
      {
         ret = cellNetCtlGetState(&state);
         if (ret < 0)
         {
            printf("cellNetCtlGetState() failed(%x)\n", ret);
            return (-1);
         }
      if (state == CELL_NET_CTL_STATE_IPObtained)
      {
         break;
      }

      sys_timer_usleep(500 * 1000);
      timeout_count--;

      if (index && timeout_count < 0)
      {
         printf("if_up_with(%d) timeout\n", index);
         return (0);
      }
      }

      sock=socket(AF_INET,SOCK_DGRAM ,0);

      target.sin_family = AF_INET;
      target.sin_port = htons(3490);
      inet_pton(AF_INET, "192.168.1.7", &target.sin_addr);

   return (0);
}

int if_down(int sid)
{
      (void)sid;
      cellNetCtlTerm();
   return (0);
}

void net_init()
{
      int ret;

      ret = cellSysmoduleLoadModule(CELL_SYSMODULE_NET);
      ret = sys_net_initialize_network();
      g_sid = if_up_with(1);
}

void net_shutdown()
{
         if_down(g_sid);
         sys_net_finalize_network();
         cellSysmoduleUnloadModule(CELL_SYSMODULE_NET);
}

void net_send(const char *__format,...)
{
         va_list args;

         va_start(args,__format);
         vsnprintf(sendbuf,4000,__format, args);
         va_end(args);

         int len=strlen(sendbuf);
         sendto(sock,sendbuf,len,MSG_DONTWAIT,(const sockaddr*)&target,sizeof(target));
}
