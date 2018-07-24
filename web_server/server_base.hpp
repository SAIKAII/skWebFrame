#ifndef SERVER_BASE_HPP
#define SERVER_BASE_HPP

#include <boost/asio.hpp>

#include <regex>
#include <unordered_map>
#include <thread>
#include <vector>

namespace skWeb{
    struct Request{
        std::string method, path, http_version;
        std::shared_ptr<std::istream> content;
        std::unordered_map<std::string, std::string> header;
        //用正则表达式处理路径匹配
        std::smatch path_match;
    };

    typedef std::map<std::string, std::unordered_map<std::string, 
            std::function<void(std::ostream&, Request&)>>> resource_type;

    //socket_type为HTTP或HTTPS
    template<typename socket_type>
    class ServerBase{
        public:
            ServerBase(unsigned short port, size_t num_threads=1):
                endpoint(boost::asio::ip::tcp::v4(), port),
                acceptor(m_io_service, endpoint),
                num_threads(num_threads){}

            //启动服务器
            void start();

            //用于服务器访问资源处理方式
            resource_type resource;
            //用于保存默认资源的处理方式
            resource_type default_resource;

        protected:
            virtual void accept() = 0;
            void process_request_and_respond(std::shared_ptr<socket_type> socket) const;
            //所有的资源及默认资源都会在vector尾部添加，并在start()中创建
            std::vector<resource_type::iterator> all_resources;
            //所有的异步IO事件都要通过它来分发处理
            //所以需要IO的对象的构造函数，都需要传入一个io_service对象
            boost::asio::io_service m_io_service;
            boost::asio::ip::tcp::endpoint endpoint;
            boost::asio::ip::tcp::acceptor acceptor;
            
            //实现线程池
            size_t num_threads;
            std::vector<std::thread> threads;
    };

    void ServerBase::start(){
        //默认资源放在vector的末尾，用作默认应答
        //默认的请求会在找不到匹配请求路径时，进行访问，所以放在最后
        for(auto it = resource.begin(); it != resource.end(); ++it){
            all_resources.push_back(it);
        }
        for(auto it = default_resource.begin(); it != default_resource.end(); ++it){
            all_resources.push_back(it);
        }

        //多态函数，子类实现逻辑
        accept();

        //创建num_threads-1个线程，后面用来处理任务队列里的任务
        for(size_t i = 1; i < num_threads; ++i){
            threads.emplace_back([this](){
                m_io_service.run();
            });
        }

        //主线程
        m_io_service.run();

        //等待其他线程，如果有的话，就等待这些线程的结果
        for(auto &t : threads)
            t.join();
    }

    template<typename socket_type>
    class Server : public ServerBase<socket_type>{};
}
#endif
