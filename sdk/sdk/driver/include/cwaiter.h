#pragma once
#include "cevent.h"

namespace rpos
{
    namespace net
    {
        namespace hokuyoLidar
        {
            template<typename ResultT>
            class CWaiter : public CEvent
            {
            public:
                CWaiter() 
                    : CEvent()
                {
                }

                ~CWaiter() 
                {}

                unsigned int waitForResult(ResultT& result,unsigned int timeout = 10000)
                {
                    try
                    {
                        if (CEvent::EVENT_OK == wait(timeout))
                        {
                            result = _result;
                        }
                    }
                    catch(...)
                    {
                        return CEvent::EVENT_FAILED;
                    }
                    return  CEvent::EVENT_TIMEOUT;
                }

                void setResult(ResultT result)
                {
                    this->_result = result;
                    set();
                }

                volatile ResultT _result;
            };
        }
    }
}