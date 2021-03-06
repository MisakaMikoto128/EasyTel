#ifndef _SIMPLE_DPP_H_
#define _SIMPLE_DPP_H_
#include "ByteBuffer.h"
#include <stdarg.h>

/*C compiler standard support*/
//#define SIMPLEDPP_SUPPORT_C89
#define SIMPLEDPP_SUPPORT_C99

#define VAR_ARG_END ((void *)0)

//cast char * to byte *
//TODO : typedef conflict or macro pollution
#ifndef sdp_byte
#define sdp_byte unsigned char
#endif
/*Cast Macro*/
/*
simple naming rule:
cast any pointer to byte pointer:
CAST_PB(): PB:byte*
CAST_PV(): PV:void*
*/
#define CAST_ANY_PTR_TO_BYTE_PTR(ptr) (sdp_byte *)(ptr)
#define CAST_ANY_PTR_TO_VOID_PTR(ptr) (void *)(ptr)
#define BYTE_PTR_CAST (sdp_byte *)
#define CAST_PB CAST_ANY_PTR_TO_BYTE_PTR

// define SimpleDPP receive error code
// level 0:
#define SIMPLEDPP_RECEIVE_ERROR -1
#define SIMPLEDPP_SENDFAILED -2  // USING,SEND ONLY USING THIS ERROR CODE
#define SIMPLEDPP_NORMAL 0  
// level 1:
#define SIMPLEDPP_ERROR_REV_OVER_CAPACITY -11 //USING
#define SIMPLEDPP_ERROR_SEND_OVER_CAPACITY -12 
// level 2:
#define SIMPLEDPP_ERROR_REV_SOH_WHEN_WAIT_END -21 //USING
#define SIMPLEDPP_ERROR_REV_NONCTRL_BYTE_WHEN_WAIT_CTRL_BYTE -22 //USING
#define SIMPLEDPP_CRC_CHECK_ERROR -23 

// SimpleDPP receive state machine's states
#define SIMPLEDPP_REV_WAIT_START 0
#define SIMPLEDPP_REV_WAIT_END 1
#define SIMPLEDPP_REV_WAIT_CTRL_BYTE 2

typedef int SimpleDPPERROR;
// SimpleDPP frame control byte (The frame delimiter)
#define SOH 0x01 //DEC: 1
#define EOT 0x04 //DEC: 4
#define ESC 0x18 //DEC: 27
#define containSimpleDPPCtrolByte(c) ((c) == SOH || (c) == EOT || (c) == ESC)

// A tag with an implementation function or variable
#define __unimplemented 
#define __implemented

//default buffer size
#define SIMPLEDDP_DEFAULT_BUFFER_SIZE 1024

typedef  void (*SimpleDPPRecvCallback_t)(void * callback_arg,const sdp_byte *data, int len);
typedef  void (*SimpleDPPRevErrorCallback_t)(void * callback_arg,SimpleDPPERROR error_code);
typedef  sdp_byte (*SimpleDPP_putchar_t)(sdp_byte c);

/* SimpleDPP Class Structure */
/**
 * SimpleDPP name rule:
 * sdp : pointer of SimpleDPP
 * sdp_o: object of SimpleDPP  
 */
typedef struct SimpleDPP_ {
    ByteBuffer send_buffer;
    ByteBuffer recv_buffer;
    int SimpleDPPErrorCnt;
    int SimpleDPPRevState;
    SimpleDPPRecvCallback_t SimpleDPPRecvCallback;
    SimpleDPPRevErrorCallback_t SimpleDPPRevErrorCallback;
    SimpleDPP_putchar_t SimpleDPP_putchar;
    void * callback_arg;
} SimpleDPP,*pSimpleDPP;


// Externally provided methods
void SimpleDPP_Constructor(SimpleDPP* sdp,sdp_byte *send_buffer,int send_buffer_capacity,sdp_byte *recv_buffer,int recv_buffer_capacity,SimpleDPPRecvCallback_t SimpleDPPRecvCallback,SimpleDPPRevErrorCallback_t SimpleDPPRevErrorCallback,SimpleDPP_putchar_t SimpleDPP_putchar,void * callback_arg);
void SimpleDPP_Destructor(SimpleDPP* sdp);


int SimpleDPP_send_datas_start(SimpleDPP* sdp);
int SimpleDPP_send_datas_add(SimpleDPP* sdp,const sdp_byte *data, int len);
int SimpleDPP_send_datas_end(SimpleDPP* sdp);

int SimpleDPP_send(SimpleDPP* sdp,const sdp_byte *data, int len);
int __SimpleDPP_send_datas(SimpleDPP* sdp,const sdp_byte *data,int data_len,...);
#ifdef SIMPLEDPP_SUPPORT_C99
#define SimpleDPP_send_datas(sdp,var_arg,...) __SimpleDPP_send_datas(sdp,var_arg,##__VA_ARGS__,VAR_ARG_END)
#elif defined SIMPLEDPP_SUPPORT_C89
#define SimpleDPP_send_datas    __SimpleDPP_send_datas
#endif


void SimpleDPP_parse(SimpleDPP* sdp,sdp_byte c);


int getSimpleDPPErrorCnt(SimpleDPP* sdp);
#endif // _SIMPLE_DPP_H_