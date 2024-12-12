#include "HttpConnection.h"

// socketû�п������죬ֻ���ƶ����캯��
HttpConnection::HttpConnection(tcp::socket socket): _socket(std::move(socket))
{
}

void HttpConnection::Start()
{
	auto self = shared_from_this();
	// ��һ������Ϊ�첽�ɶ������������������Ϊsocket.
	// �ڶ�������Ϊһ��buffer�������洢���ܵ����ݣ���Ϊhttp�ɽ����ı���ͼ����Ƶ�ȶ�����Դ�ļ���������Dynamic��̬���͵�buffer��
	// ���������������������һ��ҲҪ�����ܽ��ܶ�����Դ���͵����������
	// ���ĸ�����Ϊ�ص����������ܳɹ�����ʧ�ܣ����ᴥ���ص���������lambda���ʽ
	http::async_read(_socket, _buffer, _request, [self](beast::error_code ec, std::size_t bytes_transferred) {
		try
		{
			// error_code���ͱ�������ֱ�ӵ���bool���ʹ�����Ϊ��������operator bool()
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
