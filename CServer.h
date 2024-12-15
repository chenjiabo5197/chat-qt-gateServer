#pragma once

#include "const.h"

// 奇异递归
class CServer: public std::enable_shared_from_this<CServer>
{
public:
	// ioc，上下文，用于和底层通信，没有拷贝构造和拷贝赋值，所以这里的形参是引用类型
	CServer(boost::asio::io_context& ioc, unsigned short& port);
	void Start();
private:
	// 接收器，用于接收对端的连接
	tcp::acceptor _acceptor;
	// 上下文 所有异步操作的核心
	net::io_context& _ioc;
	//// 复用socket，接收对端的连接信息
	//tcp::socket _socket;
};

