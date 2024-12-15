#include "CServer.h"
#include "HttpConnection.h"
#include "AsioIOServicePool.h"

// _iocΪ�������ͣ�����Ĺ��캯����ʼ���б��ж�_ioc���г�ʼ����������Ϊ����һ������ͱ��뱻��ʼ������֮�����ٸı������õĶ���
CServer::CServer(boost::asio::io_context& ioc, unsigned short& port): _ioc(ioc), _acceptor(ioc, tcp::endpoint(tcp::v4(), port))
{

}

void CServer::Start()
{
	// ��ֹCServer���������޷�����Start����
	auto self = shared_from_this();
	auto& io_context = AsioIOServicePool::GetInstance()->GetIOService();
	std::shared_ptr<HttpConnection> new_conn = std::make_shared<HttpConnection>(io_context);
	// �첽������������  �����ӱ�����ʱ��������lambda�ص�����
	_acceptor.async_accept(new_conn->GetSocket(), [self, new_conn](beast::error_code ec) {
		try
		{
			// ��������socket���ӣ�����������������
			if (ec)
			{
				self->Start();
				return;
			}
			// ���������ӣ�������HttpConnection������������
			// ʹ��std::move��_socketת��Ϊ��ֵ����,��Ϊ_socketû�п������캯��
			//std::make_shared<HttpConnection>(std::move(self->_socket))->Start();
			new_conn->Start();
			// ����������������
			self->Start();
		}
		catch (std::exception& exp)
		{

		}
	});
}
