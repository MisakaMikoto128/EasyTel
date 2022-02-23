#include "EasyTel.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
EasyTelPoint etp_o;
#define SIMPLE_DPP_REV_BUFFER_SIZE 512
#define SIMPLE_DPP_SEND_BUFFER_SIZE 512
__implemented sdp_byte __send_data[SIMPLE_DPP_SEND_BUFFER_SIZE];
__implemented sdp_byte __recv_data[SIMPLE_DPP_REV_BUFFER_SIZE];
#define UNUSED(x) ((void)(x))
__implemented sdp_byte SimpleDPP_putchar(sdp_byte c)
{
    //这样接收发送串联在一起会形成一个循环，无法跳出内部接收回调，无法情况接收缓存，最后缓存满了就一直循环。
    printf("%#x\r\n", c);
    SimpleDPP_parse(&etp_o.sdp_o,c);
    return c;
}

void test(const bu_byte *data, bu_uint32 len)
{
    printf("test\n");
}

void EasyTel_ThreadDelay(int ms)
{
    usleep(ms * 1000);
}

int main(void)
{
    EasyTelPoint_Constructor(&etp_o, __send_data, SIMPLE_DPP_SEND_BUFFER_SIZE, __recv_data, SIMPLE_DPP_REV_BUFFER_SIZE, SimpleDPP_putchar);

    registerCmdCallback(&etp_o, 0x05, test);

    EasyTel_AsMaster_FindPeer_Thread(&etp_o);

    return 0;
}
