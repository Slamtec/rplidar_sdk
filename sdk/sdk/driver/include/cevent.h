#pragma once
#include <boost/thread.hpp>

namespace rpos
{
	namespace net
	{
		namespace hokuyoLidar
		{
			class CEvent
			{
			public:
				enum
				{
					EVENT_OK = 1,
					EVENT_TIMEOUT = -1,
					EVENT_FAILED = 0,
				};
			public:
				CEvent(bool bAutoReset = true, bool bSignal = false):
				  signal_(bSignal),
				  autoReset_(bAutoReset)
				  {

				  }
			public:
				unsigned long wait( unsigned int timeout = 0xFFFFFFFF )
				{
					boost::unique_lock<boost::mutex> lock(mutex_);
					if(!signal_)
					{
						if (!condition_.timed_wait(lock,boost::posix_time::milliseconds(timeout)))
						{
							return EVENT_TIMEOUT;
						}
					}
					if (autoReset_)
					{
						signal_ = false;
					} 
					return EVENT_OK;
				}
				void set(bool signal = true)
				{
					boost::lock_guard<boost::mutex> lock(mutex_);
					signal_ = signal;
					condition_.notify_one(); 
				}
			private:
				bool autoReset_;
				bool signal_;
				boost::mutex mutex_;
				boost::condition_variable condition_;
			};
		}
	}
}