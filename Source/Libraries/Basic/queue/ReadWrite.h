/*!
	@file include/CorsacBase/queue/ReadWrite.h
	@brief Implementation of ReaderWriter queue
	@date 20/12/2021
*/
#pragma once

#include "IQueue.h"
#include <readerwriterqueue/readerwriterqueue.h>
#include <thread>

namespace CorsacBase
{
	/*!
		@class RWQueue
		An implementation of ReaderWriter queue
	*/
	template <typename T>
	class RWQueue final : public IQueue<T>
	{
	public:
		/*!
			Constructs a queue with the specified handler callback
			@param handler The handler of the queue
		*/
		RWQueue(QueueHandler<T> handler) : IQueue<T>(handler), m_bShutdown(false)
		{
			StartHandlerThread();
		}
		
		/*!
			Waits for the queue thread to finish processing and destroyes the queue
		*/
		~RWQueue()
		{
			m_bShutdown = true;
			m_tQueueThread.join();
		}

		/*!
			Writes an object to the queue
			@param obj The object to write
			@return true if the object was inserted to the queue, otherwise false
		*/
		bool WriteToQueue(const T& obj) override
		{
			return m_pQueue.try_enqueue(obj);
		}

	private:
		/*!
			Starts the queue thread
		*/
		void StartHandlerThread()
		{
			std::thread k([this]
			{
				while (!m_bShutdown)
				{
					T ev = T();
					while (m_pQueue.try_dequeue(ev))
					{
						IQueue<T>::m_pHandler(ev);
					}
				}
			});

			m_tQueueThread = std::move(k);
		}

		/*!
			The concurrent queue itself
		*/
		moodycamel::ReaderWriterQueue<T, RING_BUFFER_SIZE> m_pQueue;

		/*!
			The queue thread
		*/
		std::thread m_tQueueThread;

		/*!
			Used to shutdown the queue
		*/
		bool m_bShutdown;
	};
}
