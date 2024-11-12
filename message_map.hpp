#ifndef MESSAGE_MAP_HPP
#define MESSAGE_MAP_HPP

#include "message.hpp"
#include <atomic>
#include <cstddef>
#include <cstdint>

constexpr int INITIAL_BUCKET_COUNT = 1024;

class MessageMap
{
private:
    struct MessageNode
    {
        uint64_t MessageId;
        message Msg;
        MessageNode* Next;

        MessageNode(uint64_t id, const message& msg)
            : MessageId(id)
            , Msg(msg)
            , Next(nullptr)
        {
        }
    };

    struct Bucket
    {
        std::atomic<MessageNode*> Head;

        Bucket()
            : Head(nullptr)
        {
        }
    };

    Bucket* Buckets;
    size_t BucketCount;

    size_t hash(uint64_t key) const
    {
        return key % BucketCount;
    }

public:
    MessageMap()
    {
        BucketCount = INITIAL_BUCKET_COUNT;
        Buckets = new Bucket[BucketCount];
    }

    ~MessageMap()
    {
        for (size_t i = 0; i < BucketCount; ++i)
        {
            MessageNode* node = Buckets[i].Head.load(std::memory_order_relaxed);
            while (node)
            {
                MessageNode* temp = node;
                node = node->Next;
                delete temp;
            }
        }
        delete[] Buckets;
    }

    bool insert(const message& msg)
    {
        int index = hash(msg.MessageId);
        Bucket& bucket = Buckets[index];

        MessageNode* newNode = new MessageNode(msg.MessageId, msg);

        while (true)
        {
            MessageNode* head = bucket.Head.load(std::memory_order_acquire);
            MessageNode* current = head;
            while (current)
            {
                if (current->MessageId == msg.MessageId)
                {
                    delete newNode;
                    return false;
                }
                current = current->Next;
            }

            newNode->Next = head;
            if (bucket.Head.compare_exchange_weak(head, newNode, std::memory_order_release,
                                                  std::memory_order_relaxed))
            {
                return true;
            }
        }
    }

    bool exists(uint64_t key) const
    {
        int index = hash(key);
        Bucket& bucket = Buckets[index];

        MessageNode* current = bucket.Head.load(std::memory_order_acquire);
        while (current)
        {
            if (current->MessageId == key)
            {
                return true;
            }
            current = current->Next;
        }
        return false;
    }
};

#endif // MESSAGE_MAP_HPP
