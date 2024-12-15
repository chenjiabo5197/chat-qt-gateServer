#pragma once

#include "const.h"

// ����ݹ�
class CServer: public std::enable_shared_from_this<CServer>
{
public:
	// ioc�������ģ����ں͵ײ�ͨ�ţ�û�п�������Ϳ�����ֵ������������β�����������
	CServer(boost::asio::io_context& ioc, unsigned short& port);
	void Start();
private:
	// �����������ڽ��նԶ˵�����
	tcp::acceptor _acceptor;
	// ������ �����첽�����ĺ���
	net::io_context& _ioc;
	//// ����socket�����նԶ˵�������Ϣ
	//tcp::socket _socket;
};

