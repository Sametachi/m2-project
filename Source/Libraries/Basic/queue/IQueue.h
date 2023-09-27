/*!
    @file include/CorsacBase/queue/IQueue.h
    @brief Basic queue interface
    @date 19/12/2021
*/
#pragma once

namespace CorsacBase
{
    /*!
        @typedef QueueHandler
        Function definition of a queue handler
    */
    template <typename T>
    using QueueHandler = std::function<void(const T&)>;

    constexpr int RING_BUFFER_SIZE = 1024;

    /*!
        @class IQueue
        Interface for rapresenting an abstract queue
    */
    template <typename T>
    class IQueue
    {
    public:

        /*!
            Constructs a queue with the specified handler callback
            @param handler The handler of the queue
        */
        IQueue(QueueHandler<T> handler) : m_pHandler(handler) {}

        /*!
            Default decostructor
        */
        virtual ~IQueue() = default;

        /*!
            Writes an object to the queue
            @param obj The object to write
            @return true if the object was inserted to the queue, otherwise false
        */
        virtual bool WriteToQueue(const T& obj) = 0;

    protected:
        /*!
            Handler of the queue objects
        */
        QueueHandler<T> m_pHandler;
    };
}
