#pragma once
#include <string>



namespace rpos
{ 
    namespace ev
    {
        template<typename MessageT>
        class MessageDispatcher;

        template<typename MessageT>
        class AbsMessageHandler
        {
        public:
            AbsMessageHandler(unsigned int eventType, unsigned int priority = 0, const char* handlerName = "_unnamed", bool noAutoRelease = false)
                : _eventType(eventType)
                , _priority(priority)
                , _handlerName(handlerName)
                , _noAutoRelease(noAutoRelease)
            {}

            virtual ~AbsMessageHandler() 
            {

            }

            virtual void handle(MessageDispatcher<MessageT>& sender, MessageT message) = 0;

            unsigned int getEventType() 
            { 
                return _eventType; 
            }
            const char* getHandlerName() 
            { 
                return _handlerName.c_str(); 
            }

            bool getIsAutoRelease() 
            { 
                return !_noAutoRelease; 
            }

        protected:
            unsigned int _eventType;
            unsigned int _priority;
            std::string _handlerName;
            bool _noAutoRelease;
        };

        template<typename HostT, typename MessageT>
        class MessageHandler : public AbsMessageHandler<MessageT>
        {
        public:
            typedef void (HostT::*HostFunc) (unsigned int  eventid, MessageDispatcher<MessageT>& sender, MessageT message);

            MessageHandler(HostT& host, HostFunc func, unsigned int  eventType, unsigned int  priority = 0, const char* handlerName = "_unnamed", bool noAutoRelease = false)
                : AbsMessageHandler<MessageT>(eventType, priority, handlerName, noAutoRelease)
                , _host(&host), _func(func)
            {}

            virtual ~MessageHandler(){}

            virtual void handle(MessageDispatcher<MessageT>& sender, MessageT message)
            {
                (_host->*_func)(this->_eventType ,sender, message);
            }

        protected:
            HostT* _host;
            HostFunc _func;
        };
    }
}
