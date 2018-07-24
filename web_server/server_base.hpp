#ifndef SERVER_BASE_HPP
#define SERVER_BASE_HPP

#include <boost/asio.hpp>

#include <regex>
#include <unordered_map>
#include <thread>

namespace skWeb{
    //socket_type为HTTP或HTTPS
    template<typename socket_type>
    class ServerBase{
        public:
            //启动服务器
            void start();

        protected:
            virtual void accept(){}
            void process_request_and_respond(std::shared_ptr<socket_type> socket) const;
    };

    template<typename socket_type>
    class Server : public ServerBase<socket_type>{};
}
#endif
