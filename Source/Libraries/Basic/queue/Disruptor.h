/*!
    @file include/CorsacBase/queue/Disruptor.h
    @brief Implementation of a disruptor queue
    @date 20/12/2021
*/
#pragma once

#include "IQueue.h"
#include <Disruptor/Disruptor.h>
#include <Disruptor/ThreadPerTaskScheduler.h>

namespace CorsacBase
{
    /*!
        @class DisruptorHandler
        A decorator that wraps Disruptor event handler with IQueue event handler
    */
    template <typename T>
    class DisruptorHandler final : public Disruptor::IEventHandler<T>
    {
    public:
        /*!
            Constructs a disruptor queue handler with the specified IQueue<T> handler
            @param handler The handler to use for handling the queue
        */
        DisruptorHandler(QueueHandler<T>& handler) : m_pHandler(handler) {}

        /*!
            Called when a publisher has committed an event to the RingBuffer<T>
            @param data Data committed to the RingBuffer<T>
            @param sequence Sequence number committed to the RingBuffer<T>
            @param endOfBatch flag to indicate if this is the last event in a batch from the RingBuffer<T>
        */
        void onEvent(T& data, std::int64_t sequence, bool endOfBatch) override
        {
            m_pHandler(data);
        }

    private:
        /*!
            Real queue handler
        */
        QueueHandler<T> m_pHandler;
    };

    /*!
        @class DisruptorQueue
        An implementation of the Disruptor queue
    */
    template <typename T>
    class DisruptorQueue final : public IQueue<T>
    {
    public:
        /*!
            Constructs a queue with the specified handler callback
            @param handler The handler of the queue
        */
        DisruptorQueue(QueueHandler<T> handler) : IQueue<T>(handler)
        {
            m_pTaskScheduler = std::make_shared<Disruptor::ThreadPerTaskScheduler>();
            m_pDisruptorHandler = std::make_shared<DisruptorHandler<T>>(handler);
			m_pDisruptor = std::make_shared<Disruptor::disruptor<T>>([]() { return T(); }, RING_BUFFER_SIZE, m_pTaskScheduler);
            m_pDisruptor->handleEventsWith(m_pDisruptorHandler);
			m_pTaskScheduler->start();
			m_pDisruptor->start();
        }

        /*!
            Shuts down the disruptor queue
        */
        ~DisruptorQueue()
        {
            m_pDisruptor->shutdown();
            m_pTaskScheduler->stop();
        }

        /*!
            Writes an object to the queue
            @param obj The object to write
            @return true if the object was inserted to the queue, otherwise false
        */
        bool WriteToQueue(const T& obj) override
        {
			auto buffer = m_pDisruptor->ringBuffer();
			const auto seq = buffer->next();
			(*buffer)[seq] = obj;
			buffer->publish(seq);
            return true;
        }

    private:
        /*!
            The queue thread task scheduler
        */
        std::shared_ptr<Disruptor::ThreadPerTaskScheduler> m_pTaskScheduler;

        /*!
            The disruptor queue itself
        */
        std::shared_ptr<Disruptor::disruptor<T>> m_pDisruptor;

        /*!
            Disruptor queue handler
        */
        std::shared_ptr<DisruptorHandler<T>> m_pDisruptorHandler;
        
    };
}