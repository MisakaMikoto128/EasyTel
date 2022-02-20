#include "EasyTel.h"

#include "EasyTel.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
__implemented byte SimpleDPP_putchar(byte c)
{
    //这样接收发送串联在一起会形成一个循环，无法跳出内部接收回调，无法情况接收缓存，最后缓存满了就一直循环。
    // printf("%#x\r\n", c);
    // SimpleDPP_parse(c);
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
    EasyTelPoint etp;

    EasyTelPoint_Constructor(&etp);

    registerCmdCallback(&etp, 0x05, test);

    EasyTel_AsMaster_FindPeer_Thread(&etp);

    return 0;
}
