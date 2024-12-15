#include <iostream>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "CServer.h"
#include "ConfigMgr.h"

int main()
{
    ConfigMgr gCfgMgr;
    std::string gate_port_str = gCfgMgr["GateServer"]["Port"];
    unsigned short gate_port = atoi(gate_port_str.c_str());

    try
    {
        unsigned short port = static_cast<unsigned short> (8080);
        // 底层一个线程
        net::io_context ioc{ 1 };
        // 声明一个信号集，捕获SIGINT和SIGTERM
        boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
        // 信号集异步等待
        signals.async_wait([&ioc](const boost::system::error_code& error, int signal_number) {
            if (error)
            {
                return;
            }
            // 收到停止信号后，停止ioc
            ioc.stop();
        });
        // 启动server
        std::make_shared<CServer>(ioc, port)->Start();
        std::cout << "Gate Server listen on port=" << port << std::endl;
        // 让ioc轮询起来
        ioc.run();
    }
    catch (std::exception const& e)
    {
        std::cout << "Error=" << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}