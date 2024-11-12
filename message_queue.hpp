#ifndef MESSAGE_QUEUE_HPP
#define MESSAGE_QUEUE_HPP

#include "message.hpp"
#include <atomic>

struct QueueNode
{
    message msg;
    std::atomic<QueueNode*> next;

    QueueNode(const message& m = message())
        : msg(m)
        , next(nullptr)
    {
    }
};

class MessageQueue
{
private:
    std::atomic<QueueNode*> head;
    std::atomic<QueueNode*> tail;

public:
    MessageQueue()
    {
        QueueNode* dummy = new QueueNode();
        head.store(dummy);
        tail.store(dummy);
    }

    ~MessageQueue()
    {
        message m;
        while (dequeue(m))
        {
        }
        QueueNode* dummy = head.load();
        delete dummy;
    }

    void enqueue(const message& m)
    {
        QueueNode* node = new QueueNode(m);
        node->next.store(nullptr, std::memory_order_relaxed);

        QueueNode* prev_tail = nullptr;
        while (true)
        {
            prev_tail = tail.load(std::memory_order_acquire);
            QueueNode* null_node = nullptr;
            if (prev_tail->next.compare_exchange_weak(null_node, node, std::memory_order_release,
                                                      std::memory_order_relaxed))
            {
                break;
            }
            else
            {
                tail.compare_exchange_weak(prev_tail, prev_tail->next.load(),
                                           std::memory_order_release, std::memory_order_relaxed);
            }
        }
        tail.compare_exchange_weak(prev_tail, node, std::memory_order_release,
                                   std::memory_order_relaxed);
    }

    bool dequeue(message& m)
    {
        while (true)
        {
            QueueNode* current_head = head.load(std::memory_order_acquire);
            QueueNode* current_tail = tail.load(std::memory_order_acquire);
            QueueNode* next_node = current_head->next.load(std::memory_order_acquire);

            if (current_head == current_tail)
            {
                if (next_node == nullptr)
                {
                    return false;
                }
                tail.compare_exchange_weak(current_tail, next_node, std::memory_order_release,
                                           std::memory_order_relaxed);
            }
            else
            {
                if (next_node == nullptr)
                {
                    continue;
                }
                if (head.compare_exchange_weak(current_head, next_node, std::memory_order_release,
                                               std::memory_order_relaxed))
                {
                    m = next_node->msg;
                    delete current_head;
                    return true;
                }
            }
        }
    }
};

#endif // MESSAGE_QUEUE_HPP