#pragma once 
#include <rpos/system/util/tcp_client.h>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <rpos/system/signal.h>
#include "cwaiter.h"
namespace rpos
{
    namespace net
    {
        namespace hokuyoLidar
        {
            class RplidarTcpClientHandler : public rpos::system::util::TcpClient<RplidarTcpClientHandler>::EmptyTcpClientHandler
            {
            public:
                typedef rpos::system::util::TcpClient<RplidarTcpClientHandler>::Pointer Pointer;
            public:
                virtual void onHostResolveFailure(Pointer client, const boost::system::error_code& ec);
                virtual void onConnected(Pointer client, boost::asio::ip::basic_resolver_iterator<boost::asio::ip::tcp> connectedEntry) ;
                virtual void onConnectionFailure(Pointer client, const boost::system::error_code& ec);
                virtual void onSendError(Pointer client, const boost::system::error_code& ec);
                virtual void onSendComplete(Pointer client);
                virtual void onReceiveError(Pointer client, const boost::system::error_code& ec);
                virtual void onReceiveComplete(Pointer client, const unsigned char* buffer, size_t readBytes);
                virtual void onConnectionClosed(Pointer client);

            };


            class RplidarTcpClient : public rpos::system::util::TcpClient <RplidarTcpClientHandler>
            {
            public:
                RplidarTcpClient()
                {

                }
                ~RplidarTcpClient()
                {

                }
            public:
                bool establishConnection(const std::string& addr,unsigned int port,unsigned int timeOut = 3000);
                void setRecivedMsgHandler(const boost::function<void(const unsigned char*, size_t readBytes)>& handler);
                void setLostConnectionHandler(const boost::function<void()>& handler);
            private:
                void onConect(bool success);
                void onClosed();
                void onSend(bool succsss);
                void onRecived(bool success,const unsigned char* buffer = NULL, size_t readBytes = 0);
            private:
                friend class RplidarTcpClientHandler;
            private:
                rpos::system::Signal<void(const unsigned char*, size_t readBytes)> signalMessageArrived_;
                rpos::system::Signal<void()> signalLostConnection_;
                CWaiter<bool> _connect_waiter;
            };

        }
    }
}
