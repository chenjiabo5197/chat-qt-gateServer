#pragma once

#include "const.h"

class HttpConnection: public std::enable_shared_from_this<HttpConnection>
{
public:
	friend class LogicSystem;
	HttpConnection(tcp::socket socket);
	void Start();
private:
	// ��ⳬʱ
	void CheckDeadline();
	// Ӧ����
	void WriteResponse();
	// ��������
	void HandleReq();

	tcp::socket _socket;
	// �������ݻ�����
	beast::flat_buffer _buffer{ 8192 };
	// ��������
	http::request<http::dynamic_body> _request;
	// �ظ�
	http::response<http::dynamic_body> _response;
	// ��ʱ����������,�󶨵Ķ�ʱ����_socket�Ķ�ʱ������Ϊ60s��ʱ
	net::steady_timer deadline_{
		_socket.get_executor(), std::chrono::seconds(60)
	};
	std::string _get_url;
	// ����get�����url�д��Ĳ���������Ϊmap
	std::unordered_map<std::string, std::string> _get_params;
	// ����get�����еĲ���
	void PreParseGetParam();
};

