#pragma once
#include "message_handler.h"
#include <map>
#include <iostream>
#ifndef _multi_thread
#define _multi_thread
#endif
#ifndef _single_thread
#define _single_thread
#endif
#include <boost/thread.hpp>
using namespace std;

namespace rpos
{ 
    namespace ev
    {
        template<typename MessageT>
        class MessageDispatcher 
        {
        public:
            MessageDispatcher()
            {
                removeAllListeners();
            }

            virtual ~MessageDispatcher()
            {
                removeAllListeners();
            }

            void removeAllListeners()
            {
                boost::lock_guard<boost::mutex> guard(_mapLocker);
                typename multimap<unsigned int, AbsMessageHandler<MessageT>* >::iterator iter = _handlerMap.begin();
                for(;iter != _handlerMap.end(); iter++)
                {
                    if (iter->second->getIsAutoRelease())
                    {
                        ReleaseListenerHandler(iter->second);
                    }

                }
                _handlerMap.clear();
            }

            void removeListeners(unsigned int eventType)
            {
                boost::lock_guard<boost::mutex> guard(_mapLocker);

                typename multimap<unsigned int,AbsMessageHandler<MessageT>*>::iterator iter = _handlerMap.lower_bound(eventType);
                for(;iter != _handlerMap.upper_bound(eventType); iter++) 
                {
                    delete iter->second;
                }

                iter = _handlerMap.find(eventType);
                _handlerMap.erase(iter, _handlerMap.end());
            }

            template <typename HostT> 
            static AbsMessageHandler<MessageT>*  CreateListenerHandler(
                unsigned int  eventId, 
                void (HostT::*func) (unsigned inteventid, MessageDispatcher<MessageT>& sender, MessageT message),
                HostT* pThis, unsigned int priority = 0, 
                const char* handlerName = "_unnamed",
                bool noAutoRelease = true)
            {
                AbsMessageHandler<MessageT>* eventHandler = new MessageHandler<HostT, MessageT>(*pThis, func, eventId, priority, handlerName, noAutoRelease);
                return eventHandler;
            }


            static void ReleaseListenerHandler(AbsMessageHandler<MessageT>* handler)
            {
                delete handler;
            }

            template <typename HostT>
            AbsMessageHandler<MessageT>* _multi_thread addListener(
                unsigned int  eventId,
                void (HostT::*func) (unsigned int  eventid, MessageDispatcher<MessageT>& sender, MessageT message),
                HostT* pThis, unsigned int  priority = 0, 
                const char* handlerName = "_unnamed")
            {

                AbsMessageHandler<MessageT>* eventHandler = CreateListenerHandler<HostT>(eventId, func, pThis, priority, handlerName, false);
                if (registerEventHandler(eventHandler))
                {
                    return eventHandler;
                }
                else
                {
                    ReleaseListenerHandler(eventHandler);
                    return NULL;
                }
            }

            bool _multi_thread registerEventHandler(AbsMessageHandler<MessageT>* handler)
            {
                boost::lock_guard<boost::mutex> guard(_mapLocker);
                _handlerMap.insert(pair<unsigned int , AbsMessageHandler<MessageT>*>(handler->getEventType(), handler));
                return true;
            }

            bool _multi_thread unregisterEventHandler(AbsMessageHandler<MessageT>* handler)
            {
                boost::lock_guard<boost::mutex> guard(_mapLocker);

                typename multimap<unsigned int , AbsMessageHandler<MessageT>*>::iterator iter = _handlerMap.find(handler->getEventType());
                for(;iter != _handlerMap.end(); iter++)
                {
                    if (iter->second == handler) 
                    {
                        if (iter->second->getIsAutoRelease())
                            ReleaseListenerHandler(iter->second);
                        _handlerMap.erase(iter);
                        break;
                    }
                }
                return true;
            }

            void _multi_thread dispatchMessage(unsigned int  eventId, MessageT message)
            {
                boost::lock_guard<boost::mutex> guard(_mapLocker);
                typename multimap<unsigned int, AbsMessageHandler<MessageT>*>::iterator iter = _handlerMap.lower_bound(eventId);
                for(;iter != _handlerMap.upper_bound(eventId); iter++) 
                {
                    iter->second->handle(*this, message);
                }
            }

            size_t _multi_thread getListenerCount( unsigned int  eventId )
            {
                boost::lock_guard<boost::mutex> guard(_mapLocker);
                size_t ans = _handlerMap.count(eventId);
                return ans;
            }
        protected:
            multimap<unsigned int, AbsMessageHandler<MessageT>*> _handlerMap;
            boost::mutex _mapLocker;
        };

    }
}
