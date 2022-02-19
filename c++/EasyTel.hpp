
#ifndef _EASYTEL_H_
#define _EASYTEL_H_
#include "BytesUtil.hpp"
#include "SimpleDPP.hpp"
#include <vector>
#include <functional>
#include <thread>
#define DEBUG

#ifdef DEBUG
#include <iostream>
using namespace std;
#endif

typedef std::function<void(const bu_byte *data, bu_uint32 len)> EasyTelCmdCallback;

/*
命令名命规则：
Q_开头，表示询问命令
R_开头，表示回复命令
*/

class EasyTelPoint
{

public:
    static constexpr bu_uint8 Q_EXIST_POINT = 0x00;
    static constexpr bu_uint8 R_EXIST_POINT = 0x01;
    static constexpr bu_uint8 Q_ENDIAN = 0x02;
    static constexpr bu_uint8 R_ENDIAN = 0x03;
    static constexpr bu_uint8 CMD_MAX = 0xFF;

private:
    Endian endian;
    bool need_to_change_endian;
    bool found_point;

    std::function<void(const bu_byte *data, bu_uint32 len)> callback_list[0xFF];
    SimpleDPP &sdp;
    std::thread *rev_thread = nullptr;

    // thread control
    bool close_rev_thread = false;

public:
    EasyTelPoint(SimpleDPP &sdp_) : sdp(sdp_)
    {
        endian = BytesUtil::getSelfEndian();
        need_to_change_endian = false;
        found_point = false;

        for (int i = 0; i < 0xFF; i++)
        {
            callback_list[i] = nullptr;
        }
        sdp.bindRecvCallback(this, &EasyTelPoint::SimpleDPPRecvCallback);
        sdp.bindRevErrorCallback(this, &EasyTelPoint::SimpleDPPRevErrorCallback);
    }
    ~EasyTelPoint()
    {
        if (rev_thread != nullptr)
        {
            rev_thread->join();
            delete rev_thread;
        }
        close_rev_thread = true;
    }
    /**
     * @brief
     *
     * @tparam T
     * @param cmd
     * @param obj
     * @param callback : callback function to handle the data.
     * @return true : if register success.
     * @return false
     */
    template <typename T>
    bool registerCmdCallback(const bu_byte cmd, T *obj, void (T::*callback)(const bu_byte *data, bu_uint32 len))
    {
        if (callback == nullptr || cmd <= R_ENDIAN || cmd > CMD_MAX)
        {
            return false;
        }
        else
        {
            auto lambda = [obj, callback](const bu_byte *data, bu_uint32 len)
            { (obj->*callback)(data, len); };
            callback_list[cmd] = lambda;
            return true;
        }
    }

    bool send(bu_byte cmd, const char *data = nullptr, bu_uint32 len = 0)
    {
        bu_uint32 send_len = sdp.send_datas(&cmd, sizeof(cmd), data, len);
        return send_len > 0;
    }

    void SimpleDPPRecvCallback(const std::vector<byte> &revdata)
    {
        bu_uint8 cmd = revdata[0];

        switch (cmd)
        {
        case Q_EXIST_POINT:
        {
            char endian_ = static_cast<char>(endian);
            send(R_EXIST_POINT, &endian_, sizeof(endian_));
        }

        break;
        case R_EXIST_POINT:
        {
            Endian peer_endian = (Endian)revdata[1];
            need_to_change_endian = (peer_endian != endian);
            found_point = true;
        }

        break;
        case Q_ENDIAN:
            send(R_ENDIAN);
            break;
        case R_ENDIAN:
        {
            Endian peer_endian = (Endian)revdata[1];
            need_to_change_endian = (peer_endian != endian);
        }

        break;
        default:
            if (callback_list[cmd] != nullptr)
            {
                callback_list[cmd](revdata.data() + 1, revdata.size() - 1);
            }
            break;
            break;
        }
    }

    void SimpleDPPRevErrorCallback(SimpleDPPERROR error_code)
    {
        
    }

    void start()
    {

        close_rev_thread = false;
        rev_thread = new std::thread([&]()
                                     {
            while (!close_rev_thread)
            {
                bu_uint32 cnt = 0;
                while (!found_point)
                {
                    send(Q_EXIST_POINT);
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                close_rev_thread = true;
            } });

        rev_thread->detach();
    }

    void stop()
    {
        close_rev_thread = true;
    }

    bool isRunning()
    {
        return !close_rev_thread;
    }

    bool foundPoint()
    {
        return found_point;
    }
};

#endif // _EASYTEL_H_
