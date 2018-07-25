#ifndef SERVER_HTTP_HPP
#define SERVER_HTTP_HPP

#include "server_base.hpp"
namespace skWeb{
    typedef boost::asio::ip::tcp::socket HTTP;
    template<>
    class Server<HTTP> : public ServerBase<HTTP>{
        public:
            Server(unsigned short port, size_t num_threads=1):
                ServerBase<HTTP>::ServerBase(port, num_threads){};

        private:
            void accept(){
                auto socket = std::make_shared<HTTP>(m_io_service);

                acceptor.async_accept(*socket, [this, socket](const boost::system::error_code& ec){
                    //接受一个连接并等待别的连接
                    accept();
                    if(!ec){
                        process_request_and_respond(socket);
                    }
                });
            }
    };
}

#endif
