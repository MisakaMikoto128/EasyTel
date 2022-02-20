/* Includes */
#include "EasyTel.h"

/*Global */
static pEasyTelPoint easyTelPoint = NULL;

/*const variable definition */

#define Q_EXIST_POINT 0x00
#define R_EXIST_POINT 0x01
#define Q_ENDIAN      0x02
#define R_ENDIAN      0x03
#define CMD_MAX       CALLBACK_LIST_LENGTH

/*
命令名命规则：
Q_开头，表示询问命令
R_开头，表示回复命令
*/

void EasyTelPoint_Constructor(EasyTelPoint *etp)
{
    etp->endian = getSelfEndian();
    etp->need_to_change_endian = false;
    etp->found_point = false;
    for (int i = 0; i < CALLBACK_LIST_LENGTH; i++)
    {
        etp->callback_list[i] = NULL;
    }
    etp->close_rev_thread = true;
    easyTelPoint = etp;
    EasyTel_start(etp);
    SimpleDPP_init();
}

void EsayTelPoint_Destructor(EasyTelPoint *etp)
{
    etp->close_rev_thread = true;
    easyTelPoint = NULL;
}

bool registerCmdCallback(EasyTelPoint *etp, bu_uint8 cmd, EasyTelCmdCallback_C callback)
{
    if (callback == NULL || cmd <= R_ENDIAN || cmd > CMD_MAX)
    {
        return false;
    }
    else
    {
        etp->callback_list[cmd] = callback;
        return true;
    }
}

bool EasyTel_send(bu_byte cmd, const char *data, bu_uint32 len)
{
    return SimpleDPP_send_datas(&cmd, sizeof(cmd), data, len);
}

__implemented void SimpleDPPRecvCallback(const byte *data, int len)
{
    if (easyTelPoint == NULL)
    {
        return;
    }
    bu_uint8 cmd = data[0];

    switch (cmd)
    {
    case Q_EXIST_POINT:
    {
        char endian_ = (char)(easyTelPoint->endian);
        EasyTel_send(R_EXIST_POINT, &endian_, sizeof(endian_));
    }

    break;
    case R_EXIST_POINT:
    {
        Endian peer_endian = (Endian)data[1];
        easyTelPoint->need_to_change_endian = (peer_endian != easyTelPoint->endian);
        easyTelPoint->found_point = true;
    }

    break;
    case Q_ENDIAN:
        EasyTel_send(R_ENDIAN, "", 0);
        break;
    case R_ENDIAN:
    {
        Endian peer_endian = (Endian)data[1];
        easyTelPoint->need_to_change_endian = (peer_endian != easyTelPoint->endian);
    }

    break;
    default:
        if (easyTelPoint->callback_list[cmd] != NULL)
        {
            easyTelPoint->callback_list[cmd](data + 1, len - 1);
        }
        break;
        break;
    }
}

__implemented void SimpleDPPRevErrorCallback(SimpleDPPERROR error_code)
{
}

void EasyTel_start(EasyTelPoint *etp)
{
    etp->close_rev_thread = false;
}

void EasyTel_stop(EasyTelPoint *etp)
{
    etp->close_rev_thread = true;
}

bool EasyTel_isRunning(EasyTelPoint *etp)
{
    return !etp->close_rev_thread;
}

bool EasyTel_foundPoint(EasyTelPoint *etp)
{
    return etp->found_point;
}


#include <stdio.h>
/*如果使用操作系统，将其作为一个线程或者一个task,并且实现*/
/**
 * @brief 作为Master设备去发现总线上的子设备
 *
 * @param etp
 */
void EasyTel_AsMaster_FindPeer_Thread(EasyTelPoint *etp)
{
    while (!etp->close_rev_thread)
    {
        bu_uint32 cnt = 0;
        while (!etp->found_point)
        {
            EasyTel_send(Q_EXIST_POINT,"",0);
            
            EasyTel_ThreadDelay(etp->thread_delay_ms);
        }
        etp->close_rev_thread = true;
    }
    
}

/*如果无操作系统，将方法放置在大约200ms周期的定时器中断中实现扫描*/
void EasyTel_AsMaster_FindPeer_ScanMeta(EasyTelPoint *etp)
{
    if (!etp->close_rev_thread)
    {
        static bu_uint32 cnt = 0;
        if (!etp->found_point)
        {
            EasyTel_send(Q_EXIST_POINT, "", 0);
        }
        else
        {
            etp->close_rev_thread = true;
        }
    }
}
