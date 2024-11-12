#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <cstdint>

#pragma pack(push, 1)
struct message
{
    uint16_t MessageSize;
    uint8_t MessageType;
    uint64_t MessageId;
    uint64_t MessageData;
};
#pragma pack(pop)

#endif // MESSAGE_HPP
