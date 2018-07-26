#ifndef SERVER_HTTPS_HPP
#define SERVER_HTTPS_HPP

#include "server_http.hpp"
#include <boost/asio/ssl.hpp>

namespace skWeb{
    typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> HTTPS;

    template<>
    class Server<HTTPS> : public ServerBase<HTTPS>{
        public:
            Server(unsigned short port, size_t num_threads,
                    const std::string& cert_file, const std::string& private_key_file):
                ServerBase<HTTPS>::ServerBase(port, num_threads),
                context(boost::asio::ssl::context::sslv23){
                    //使用证书文件
                    context.use_certificate_chain_file(cert_file);
                    //使用私钥文件，相比之下需要传入一个参数来指明文件的格式
                    context.use_private_key_file(private_key_file, boost::asio::ssl::context::pem);
                }

        private:
            boost::asio::ssl::context context;

            //HTTPs服务器与HTTP服务器相比
            //其区别在于对socket对象的构造方式有所不同
            //HTTPS会在socket这一步中对IO流进行加密
            //因此实现的accept()方法需要对socket用ssl context初始化
            void accept(){
                auto socket = std::shared_ptr<HTTPS>(m_io_service, context);

                acceptor.async_accept((*socket).lowest_layer(),
                        [this, socket](const boost::system::error_code& ec){
                            accept();

                            //处理错误
                            if(!ec){
                                (*socket).async_handshake(boost::asio::ssl::stream_base::server,
                                        [this, socket](const boost::system::error_code& ec){
                                            if(!ec){
                                                process_request_and_respond(socket);
                                            }
                                        });
                            }
                        });
            }
    };
}
