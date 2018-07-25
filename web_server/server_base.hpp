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
            //解析请求
            Request parse_request(std::istream& stream) const;
            void respond(std::shared_ptr<socket_type> socket, std::shared_ptr<Request> request) const;

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

    void ServerBase::process_request_and_respond(std::shared_ptr<socket_type> socket) const{
        auto read_buffer = std::make_shared<boost::asio::streambuf>();

        boost::asio::async_read_until(*socket, *read_buffer, "\r\n\r\n",
                [this, socket, read_buffer](const boost::system::error_code& ec, size_t bytes_transferred){
                    if(!ec){
                        //read_buffer->size()并不一定和bytes_transferred相等，Boost文档指出：
                        //在async_read_until操作成功后，streambuf可能还包含界定符后面的一些额外数据
                        //所以较好的做法是直接从流中提取并解析当前的read_buffer左边的报头，再拼接async_read后面的内容
                        size_t total = read_buffer->size();
                        //转换到istream
                        //用缓冲区的指针所指地址来构造istream实例
                        std::istream stream(read_buffer.get());
                        auto request = std::make_shared<Request>();

                        //将stream中的请求信息进行解析，然后保存到request对象中
                        *request = parse_request(stream);

                        size_t num_additional_bytes = total - bytes_transferred;

                        //如果有该键，则继续读取
                        if(request->header.count("Content-Length")>0){
                            boost::asio::async_read(*socket, *read_buffer,
                            boost::asio::transfer_exactly(stoull(request->header["Content-Length"]) - num_additional_bytes), 
                            [this, socket, read_buffer, request](const boost::system::error_code& ec, size_t bytes_transffered){
                                if(!ec){
                                    //将指针作为istream对象存储到read_buffer中
                                    request->content = std::shared_ptr<std::istream>(new std::istream(read_buffer.get()));
                                    respond(socket, request);
                                }
                            });
                        }else{
                            respond(socket, request);
                        }
                    }
                });
    }

    Request ServerBase::parse_request(std::istream& stream) const {
      Request request;
      //使用正则表达式，解析出请求方法、请求路径及HTTP版本
      std::regex regex_line("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
      std::smatch sub_match;

      //解析第一行请求方法、路径和HTTP版本
      std::string line;
      getline(stream, line);
      //移除末尾的'\r'字符
      line.pop_back();
      if(std::regex_match(line, sub_match, regex_line)){
        request.method = sub_match[1];
        request.path = sub_match[2];
        request.http_version = sub_match[3];

         //解析头部的其他信息
        bool matched;
        regex_line = "^([^:]*): ?(.*)$";
        do{
            getine(stream, line);
            line.pop_back();
            matched = std::regex_match(line, sub_match, regex_line);
            if(true == matched){
                //比如Proxy-Connection: Keep-alive
                request.header[sub_match[1]] = sub_match[2];
            }
        }while(true == matched);
      }
      return request;
    }

    void ServerBase::respond(std::shared_ptr<socket_type> socket, std::shared_ptr<Request> request) const{
        //对请求路径和方法进行匹配查找，并生成响应
        for(auto res_it : all_resources){
            std::regex e(res_it->first);
            std::smatch sm_res;
            if(std::regex_match(request->path, sm_res, e)){
                if(res_it->second.count(request->method)>0){
                    //把sm_res内容移到path_match，移动后，sm_res就不再拥有内容
                    request->path_match = move(sm_res);
                    
                    auto write_buffer = std::make_shared<boost::asio::streambuf>();
                    std::ostream response(write_buffer.get());
                    res_it->second[request->method](response, *request);

                    //在lambda中捕获write_buffer，使其不会在async_write完成前被销毁
                    boost::asio::async_write(*socket, *write_buffer,
                            [this, socket, request, write_buffer](const boost::system::error_code& ec, size_t bytes_transffered){
                                //HTTP持久连接(HTTP 1.1)，递归调用
                                if(!ec && stof(request->http_version)>1.05){
                                    process_request_and_respond(socket);
                                }
                            });
                    return;
                }
            }
        }
    }

    template<typename socket_type>
    class Server : public ServerBase<socket_type>{};
}
#endif
