#ifndef IOTEST_HPP
#define IOTEST_HPP

#include "EasyTel.hpp"
#include <iostream>
#include <cstring>
using namespace std;


class iotest
{

    SimpleDPP sdp;
    EasyTelPoint *etp;
public:
    explicit iotest()
    {
        sdp.bindSendBuffer([&](const std::vector<byte> &senddata){
         cout << "senddata: " << endl;
         for(auto &i : senddata)
         {
             cout <<hex<< (int)i << " ";
         }
         cout << endl;
         sdp.parse(senddata); 
         });

        etp = new EasyTelPoint(sdp);
        etp->registerCmdCallback(0x05, this, &iotest::test);
        etp->start();
    }

    ~iotest()
    {
        etp->stop();
        delete etp;
    }
public:
    int exec()
    {
        while (true)
        {
            
        }
        return 0;
    }

    void test(const bu_byte *data, const bu_uint32 len)
    {
        cout << "test" << endl;
    }


};

#endif // IOTEST_HPP
