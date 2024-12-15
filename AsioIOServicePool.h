#pragma once

#include <vector>
#include <boost/asio.hpp>
#include "Singleton.h"

class AsioIOServicePool: public Singleton<AsioIOServicePool>
{
    friend Singleton<AsioIOServicePool>;
public:
    using IOService = boost::asio::io_context;
    /*
    * 1、保持 io_context 运行：当有一个或多个线程正在运行 io_context::run() 方法，但可能在某些时候没有异步操作需要处理时，
      可以通过创建 io_context::work 的实例来确保 io_context 不会退出。
      2、控制 io_context 的生命周期：通过创建和销毁 io_context::work 的实例，可以更灵活地控制 io_context 的运行和停止。
      例如，在所有的异步操作完成后，可以销毁 io_context::work 的实例来允许 io_context 正常退出
    */
    using Work = boost::asio::io_context::work;
    using WorkPtr = std::unique_ptr<Work>;
    ~AsioIOServicePool();
    AsioIOServicePool(const AsioIOServicePool&) = delete;
    AsioIOServicePool& operator=(const AsioIOServicePool&) = delete;
    // 使用 round-robin 的方式返回一个 io_service
    boost::asio::io_context& GetIOService();
    // 停止服务池
    void Stop();
private:
    // thread::hardware_concurrency() 返回cpu的核数，当前设置2个
    AsioIOServicePool(std::size_t size = 2/*std::thread::hardware_concurrency()*/);
    std::vector<IOService> _ioServices;
    std::vector<WorkPtr> _works;
    std::vector<std::thread> _threads;
    // 下一个io_context的索引
    std::size_t _nextIOService;
};

