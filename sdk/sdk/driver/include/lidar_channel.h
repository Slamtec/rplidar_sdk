#pragma once
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
#include <boost/thread.hpp>
#include "lidar_protocol.h"
#include "cwaiter.h"
#include <queue>
#include <iterator>
#include <algorithm>


namespace rpos
{
    namespace net
    {
        namespace hokuyoLidar
        {
            class LidarChannel: public rpos::ev::MessageDispatcher<lidar_message_autoptr_t &>
            {
            public:
                LidarChannel();
                virtual ~LidarChannel();
            public:
                virtual void asyncRequest(const std::vector<unsigned char>& request) = 0;
                virtual bool isConnected() = 0;
                bool syncSendMsg(const std::string& code, unsigned int start, unsigned int end, unsigned short grouping, const std::string& defineInfo="");
                bool syncSendMsg(const std::string& code, unsigned int start, unsigned int end, unsigned short grouping,unsigned char skips,unsigned short scans,const std::string& defineInfo="");
                bool syncSendMsg(const std::string& code, unsigned char control,const std::string& defineInfo="");
                bool syncSendMsg(const std::string& code, const std::string& defineInfo="");
                virtual void close() = 0;
                virtual bool bind(boost::shared_ptr<lidarProtocol> protocol);
                void stop();
            protected:
                void onMsgArrived(const unsigned char* buffer,size_t size);
            protected:
                virtual void handlerMsgDecoded(unsigned int eventid, evtDispatcher_t& endpoint, lidar_message_autoptr_t& msg);
                virtual void handlerMsgCorrupted(unsigned int eventid, evtDispatcher_t& endpoint, lidar_message_autoptr_t& msg);
            protected:
                void decoder();
                void decoderThreadProc();
            private:
                boost::shared_ptr<lidarProtocol> _protocol;
                bool _working;
                boost::mutex _rxLock; //recivelock
                std::list<std::vector<unsigned char>> _rxQueue;
                boost::shared_ptr<boost::thread> _decoderThread;
                CEvent _decoderQueueWaiter;
                std::list< evtHandler_t * > _allhandlers;
            };
        }
    }
}