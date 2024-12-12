#include "HttpConnection.h"

// socket没有拷贝构造，只有移动构造函数
HttpConnection::HttpConnection(tcp::socket socket): _socket(std::move(socket))
{
}

void HttpConnection::Start()
{
	auto self = shared_from_this();
	// 第一个参数为异步可读的数据流，可以理解为socket.
	// 第二个参数为一个buffer，用来存储接受的数据，因为http可接受文本，图像，音频等多种资源文件，所以是Dynamic动态类型的buffer。
	// 第三个参数是请求参数，一般也要传递能接受多种资源类型的请求参数。
	// 第四个参数为回调函数，接受成功或者失败，都会触发回调函数，用lambda表达式
	http::async_read(_socket, _buffer, _request, [self](beast::error_code ec, std::size_t bytes_transferred) {
		try
		{
			// error_code类型变量可以直接当做bool类型处理，因为其重载了operator bool()
			if (ec)
			{
				std::cout << "http read err=" << ec.what() << std::endl;
				return;
			}
		}
		catch ()
		{

		}
	});
}

void HttpConnection::CheckDeadline()
{
}

void HttpConnection::WriteResponse()
{
}

void HttpConnection::handleReq()
{
}
