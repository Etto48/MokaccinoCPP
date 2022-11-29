#pragma once
#include <mutex>
#include <string>
#include <queue>

class MessageQueue
{
public:
    struct MessageQueueItem
    {
        std::string src;
        std::string msg;
    };
private:
    std::mutex queue_lock;
    std::queue<MessageQueueItem> queue;
public:
    MessageQueueItem get();
    void put(const MessageQueueItem& item);
};