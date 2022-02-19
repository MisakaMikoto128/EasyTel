#ifndef SIMPLEDPP_H
#define SIMPLEDPP_H

#include <vector>
#include <functional>

// define SimpleDPP receive error code
// level 0:
#define SIMPLEDPP_RECEIVE_ERROR -1
#define SIMPLEDPP_SENDFAILED -2 // USING,SEND ONLY USING THIS ERROR CODE
#define SIMPLEDPP_NORMAL 0
// level 1:
#define SIMPLEDPP_ERROR_REV_OVER_CAPACITY -11 // USING
#define SIMPLEDPP_ERROR_SEND_OVER_CAPACITY -12
// level 2:
#define SIMPLEDPP_ERROR_REV_SOH_WHEN_WAIT_END -21                // USING
#define SIMPLEDPP_ERROR_REV_NONCTRL_BYTE_WHEN_WAIT_CTRL_BYTE -22 // USING
#define SIMPLEDPP_CRC_CHECK_ERROR -23

// cast char * to byte *
#define byte unsigned char
#define CAST_CHAR_PTR_TO_BYTE_PTR(ptr) (byte *)(ptr)

// SimpleDPP receive state machine's states
#define SIMPLEDPP_REV_WAIT_START 0
#define SIMPLEDPP_REV_WAIT_END 1
#define SIMPLEDPP_REV_WAIT_CTRL_BYTE 2

typedef int SimpleDPPERROR;
// SimpleDPP frame control byte (The frame delimiter)
#define SOH 0x01 // DEC: 1
#define EOT 0x04 // DEC: 4
#define ESC 0x18 // DEC: 27
#define containSimpleDPPCtrolByte(c) ((c) == SOH || (c) == EOT || (c) == ESC)

template <typename T>
struct RecvCallbackFun_t
{
    T *obj;
    void (T::*fun)(const byte *data, const size_t len);
    RecvCallbackFun_t()
    {
        obj = nullptr;
        fun = nullptr;
    }

    RecvCallbackFun_t(T *obj, void (T::*func)(const byte *data, const size_t len))
    {

        this->obj = obj;
        this->func = func;
    }

    void operator()(const byte *data, const size_t len)
    {
        if (isCallAbled())
        {
            (this->obj->*(this->func))(data, len);
        }
    }

    bool isCallAbled()
    {
        return this->obj != nullptr && this->func != nullptr;
    }
};

class SimpleDPP
{
private:
    std::vector<byte> sendBuffer;
    std::vector<byte> revBuffer;
    int SimpleDPPErrorCnt;
    int SimpleDPPRevState;
    constexpr static int SEND_START = 0;
    constexpr static int SENDING = 1;

    int send_stage; // 0:start, 1:sending,only used in send_datas()
private:
    void SimpleDPPRecvInnerCallback()
    {
        if (RecvCallback != nullptr)
        {
            RecvCallback(revBuffer);
        }
        revBuffer.clear();
    }

    void SimpleDPPRevErrorInnerCallback(SimpleDPPERROR error_code)
    {
        if (RevErrorCallback != nullptr)
        {
            RevErrorCallback(error_code);
        }
    }

    void send_buffer()
    {
        if (SendBuffer != nullptr)
        {
            SendBuffer(sendBuffer);
        }
    }

    std::function<void(const std::vector<byte> &revdata)> RecvCallback = nullptr;
    std::function<void(SimpleDPPERROR error_code)> RevErrorCallback = nullptr;
    std::function<void(const std::vector<byte> &senddata)> SendBuffer = nullptr;

public:
    template <class T>
    void bindRecvCallback(T *obj, void (T::*func)(const std::vector<byte> &revdata))
    {
        auto lambda = [obj, func](const std::vector<byte> &data)
        { (obj->*func)(data); };
        RecvCallback = lambda;
    }

    template <class T>
    void bindRevErrorCallback(T *obj, void (T::*func)(SimpleDPPERROR error_code))
    {
        auto lambda = [obj, func](SimpleDPPERROR error_code_)
        { (obj->*func)(error_code_); };

        RevErrorCallback = lambda;
    }

    template <class T>
    void bindSendBuffer(T *obj, void (T::*func)(const std::vector<byte> &senddata))
    {
        auto lambda = [obj, func](const std::vector<byte> &data)
        { (obj->*func)(data); };

        SendBuffer = lambda;
    }

    void bindRecvCallback(std::function<void(const std::vector<byte> &revdata)> RecvCallback)
    {
        this->RecvCallback = RecvCallback;
    }

    void bindRevErrorCallback(std::function<void(SimpleDPPERROR error_code)> RevErrorCallback)
    {
        this->RevErrorCallback = RevErrorCallback;
    }

    void bindSendBuffer(std::function<void(const std::vector<byte> &senddata)> SendBuffer)
    {
        this->SendBuffer = SendBuffer;
    }

public:
    explicit SimpleDPP()
    {
        SimpleDPPErrorCnt = 0;
        SimpleDPPRevState = SIMPLEDPP_REV_WAIT_START;
        send_stage = SEND_START;
    }

    ~SimpleDPP() {}

    void parse(const byte *data, int len)
    {
        for (int i = 0; i < len; i++)
        {
            parse(data[i]);
        }
    }
    int getSimpleDPPErrorCnt() { return SimpleDPPErrorCnt; }

    void parse(byte c)
    {
        switch (SimpleDPPRevState)
        {
        case SIMPLEDPP_REV_WAIT_START:
            if (c == SOH)
            {
                SimpleDPPRevState = SIMPLEDPP_REV_WAIT_END;
            }
            break;
        case SIMPLEDPP_REV_WAIT_END:
            switch (c)
            {
            case SOH:
                SimpleDPPRevState = SIMPLEDPP_REV_WAIT_START;
                SimpleDPPRevErrorInnerCallback(SIMPLEDPP_ERROR_REV_SOH_WHEN_WAIT_END);
                break;
            case EOT:
                SimpleDPPRevState = SIMPLEDPP_REV_WAIT_START;
                SimpleDPPRecvInnerCallback();
                break;
            case ESC:
                SimpleDPPRevState = SIMPLEDPP_REV_WAIT_CTRL_BYTE;
                break;
            default:
                revBuffer.push_back(c);
                break;
            }
            break;
        case SIMPLEDPP_REV_WAIT_CTRL_BYTE:
            if (containSimpleDPPCtrolByte(c))
            {
                revBuffer.push_back(c);
                SimpleDPPRevState = SIMPLEDPP_REV_WAIT_END;
            }
            else
            {
                SimpleDPPRevErrorInnerCallback(SIMPLEDPP_ERROR_REV_NONCTRL_BYTE_WHEN_WAIT_CTRL_BYTE);
            }
            break;
        default:
            break;
        }
    }

    int send(const byte *data, int len)
    {
        int i;
        // 1. empty buffer
        sendBuffer.clear();
        // 2. push SHO
        sendBuffer.push_back(SOH);

        for (i = 0; i < len; i++)
        {
            // 3. push message body,when encounter SOH,EOT or ESC,using ESC escape it.
            if (containSimpleDPPCtrolByte(data[i]))
            {
                // escaped control byte only 2 bytes
                sendBuffer.push_back(ESC);
                sendBuffer.push_back(data[i]);
            }
            else
            {
                sendBuffer.push_back(data[i]);
            }
        }
        // 4. push EOT
        sendBuffer.push_back(EOT);
        // 5. send message
        send_buffer();
        return sendBuffer.size();
    }

public:
    /**
     * @brief must be used before send_datas_add() and send_datas_end()
     */
    void send_datas_start()
    {
        // 1. empty buffer
        sendBuffer.clear();
        // 2. push SHO
        sendBuffer.push_back(SOH);
    }

    /**
     * @brief must be used between send_datas_start() and send_datas_add()
     */
    void send_datas_add(const byte *data, int len)
    {
        for (int i = 0; i < len; i++)
        {
            // 3. push message body,when encounter SOH,EOT or ESC,using ESC escape it.
            if (containSimpleDPPCtrolByte(data[i]))
            {
                // escaped control byte only 2 bytes
                sendBuffer.push_back(ESC);
                sendBuffer.push_back(data[i]);
            }
            else
            {
                sendBuffer.push_back(data[i]);
            }
        }
    }

    /**
     * @brief must be used after send_datas_start() and send_datas_add()
     */
    void send_datas_end()
    {
        // 4. push EOT
        sendBuffer.push_back(EOT);
        // 5. send message
        send_buffer();
    }

public:
    /**
     * @brief must be used before rev_datas_add() and rev_datas_end()
     */
    template <typename First, typename Second, typename... Rest>
    int send_datas(const First &first, const Second &second, const Rest &...rest)
    {
        // if args number is not even, return SIMPLEDPP_SENDFAILED
        if (sizeof...(rest) % 2 != 0)
        {
            return SIMPLEDPP_SENDFAILED;
        }
        const byte *data = (const byte *)first;
        int len = (int)second;
        switch (send_stage)
        {
        case SEND_START:
            send_datas_start();
            send_datas_add(data, len);
            send_stage = SENDING;
            break;
        case SENDING:
            send_datas_add(data, len);
            break;
        default:
            break;
        }
        return send_datas(rest...); // 将会根据语法来递归调用
    }

private:
    int send_datas(void)
    {
        send_datas_end();
        send_stage = SEND_START;
        return sendBuffer.size();
    }
};

#endif // SIMPLEDPP_H