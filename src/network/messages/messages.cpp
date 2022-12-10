#include "messages.hpp"
#include "../../defines.hpp"
#include "../../multithreading/multithreading.hpp"
#include "../../parsing/parsing.hpp"
#include "../../logging/logging.hpp"
#include "../MessageQueue/MessageQueue.hpp"
#include "../udp/udp.hpp"
namespace network::messages
{
    MessageQueue messages_queue;
    void messages()
    {
        while(true)
        {
            auto item = messages_queue.pull();
            auto args = parsing::msg_split(item.msg);
            if(args.size() >= 2)
                logging::recieved_text_message_log(item.src,args[1]);
        }
    }
    void init()
    {
        udp::register_queue("MSG",messages_queue,true);

        multithreading::add_service("messages",messages);
    }
    void send(const std::string& name, const std::string& msg)
    {
        udp::send(parsing::compose_message({"MSG",msg}),name);
        logging::sent_text_message_log(name,msg);
    }
}