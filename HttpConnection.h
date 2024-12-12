#pragma once

#include "const.h"

class HttpConnection: public std::enable_shared_from_this<HttpConnection>
{
public:
	HttpConnection(tcp::socket socket);
	void Start();
private:
	// 检测超时
	void CheckDeadline();
	// 应答函数
	void WriteResponse();
	// 处理请求
	void handleReq();

	tcp::socket _socket;
	// 接收数据缓冲区
	beast::flat_buffer _buffer{ 8192 };
	// 接收请求
	http::request<http::dynamic_body> _request;
	// 回复
	http::response<http::dynamic_body> _response;
	// 定时器（变量）,绑定的定时器是_socket的定时器，认为60s后超时
	net::steady_timer deadline_{
		_socket.get_executor(), std::chrono::seconds(60)
	};
};

