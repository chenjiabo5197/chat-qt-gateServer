#include "CServer.h"
#include "HttpConnection.h"
#include "AsioIOServicePool.h"

// _ioc为引用类型，在类的构造函数初始化列表中对_ioc进行初始化。这是因为引用一旦定义就必须被初始化，且之后不能再改变其引用的对象
CServer::CServer(boost::asio::io_context& ioc, unsigned short& port): _ioc(ioc), _acceptor(ioc, tcp::endpoint(tcp::v4(), port))
{

}

void CServer::Start()
{
	// 防止CServer类析构后无法调用Start函数
	auto self = shared_from_this();
	auto& io_context = AsioIOServicePool::GetInstance()->GetIOService();
	std::shared_ptr<HttpConnection> new_conn = std::make_shared<HttpConnection>(io_context);
	// 异步接受连接请求  当连接被接受时，将调用lambda回调函数
	_acceptor.async_accept(new_conn->GetSocket(), [self, new_conn](beast::error_code ec) {
		try
		{
			// 出错，放弃socket连接，继续监听其他链接
			if (ec)
			{
				self->Start();
				return;
			}
			// 创建新链接，并创建HttpConnection类管理这个连接
			// 使用std::move将_socket转换为右值引用,因为_socket没有拷贝构造函数
			//std::make_shared<HttpConnection>(std::move(self->_socket))->Start();
			new_conn->Start();
			// 继续监听其他连接
			self->Start();
		}
		catch (std::exception& exp)
		{

		}
	});
}
