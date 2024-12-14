#include "HttpConnection.h"
#include "LogicSystem.h"

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
			// http�����������Է���������ֽ���
			boost::ignore_unused(bytes_transferred);
			self->HandleReq();
			self->CheckDeadline();
		}
		catch (std::exception& exp)
		{
			std::cout << "exp=" << exp.what() << std::endl;
		}
	});
}

void HttpConnection::CheckDeadline()
{
	auto self = shared_from_this();
	deadline_.async_wait([self](beast::error_code ec) {
		if (!ec)
		{
			self->_socket.close();
		}
	});
}

void HttpConnection::WriteResponse()
{
	auto self = shared_from_this();
	_response.content_length(_response.body().size());
	// ������������һ����ɴ�����򣬵�д��������ʱ����
	http::async_write(_socket, _response, [self](beast::error_code ec, std::size_t bytes_transferred) {
		// �رշ����
		self->_socket.shutdown(tcp::socket::shutdown_send, ec);
		// ȡ����ʱ��
		self->deadline_.cancel();
	});
}

// char תΪ16����
unsigned char ToHex(unsigned char x)
{
	return  x > 9 ? x + 55 : x + 48;
}

// char��16����תΪʮ����
unsigned char FromHex(unsigned char x)
{
	unsigned char y;
	if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
	else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
	else if (x >= '0' && x <= '9') y = x - '0';
	else assert(0);
	return y;
}

// ���룬��url�к��ֲ����ַ�ת��Ϊ'%'������ʮ�������ַ�ƴ�ӡ���ƴ��'%'���ٽ��ַ��ĸ���λƴ�ӵ�strTemp�ϣ���󽫵���λƴ�ӵ�strTemp�ϣ������ַ����ֲ���
std::string UrlEncode(const std::string& str)
{
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		//�ж��Ƿ�������ֺ���ĸ����
		if (isalnum((unsigned char)str[i]) ||
			(str[i] == '-') ||
			(str[i] == '_') ||
			(str[i] == '.') ||
			(str[i] == '~'))
			strTemp += str[i];
		else if (str[i] == ' ') //Ϊ���ַ�
			strTemp += "+";
		else
		{
			//�����ַ������֣���Ҫ��ǰ��%���Ҹ���λ�͵���λ�ֱ�תΪ16����
			strTemp += '%';
			strTemp += ToHex((unsigned char)str[i] >> 4);
			strTemp += ToHex((unsigned char)str[i] & 0x0F);
		}
	}
	return strTemp;
}

 // ���룬��ת��Ϊ%+����λ������λ�ַ���ת��Ϊ����
std::string UrlDecode(const std::string& str)
{
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		//��ԭ+Ϊ��
		if (str[i] == '+') strTemp += ' ';
		//����%������������ַ���16����תΪchar��ƴ��
		else if (str[i] == '%')
		{
			assert(i + 2 < length);
			unsigned char high = FromHex((unsigned char)str[++i]);
			unsigned char low = FromHex((unsigned char)str[++i]);
			strTemp += high * 16 + low;
		}
		else strTemp += str[i];
	}
	return strTemp;
}

void HttpConnection::PreParseGetParam() {
	// ��ȡ URI  url?key=value&key=value   _request.target()��ȡ HTTP �����Ŀ�� URI
	auto uri = _request.target();
	// ���Ҳ�ѯ�ַ����Ŀ�ʼλ�ã��� '?' ��λ�ã�  
	auto query_pos = uri.find('?');
	// ��������������
	if (query_pos == std::string::npos) {
		_get_url = uri;
		return;
	}
	// substr ����ҿ�
	_get_url = uri.substr(0, query_pos);
	std::string query_string = uri.substr(query_pos + 1);
	std::string key;
	std::string value;
	size_t pos = 0;
	while ((pos = query_string.find('&')) != std::string::npos) {
		auto pair = query_string.substr(0, pos);
		size_t eq_pos = pair.find('=');
		if (eq_pos != std::string::npos) {
			key = UrlDecode(pair.substr(0, eq_pos)); // ������ url_decode ����������URL����  
			value = UrlDecode(pair.substr(eq_pos + 1));
			_get_params[key] = value;
		}
		query_string.erase(0, pos + 1);
	}
	// �������һ�������ԣ����û�� & �ָ�����  
	if (!query_string.empty()) {
		size_t eq_pos = query_string.find('=');
		if (eq_pos != std::string::npos) {
			key = UrlDecode(query_string.substr(0, eq_pos));
			value = UrlDecode(query_string.substr(eq_pos + 1));
			_get_params[key] = value;
		}
	}
}

void HttpConnection::HandleReq()
{
	// ���ð汾
	_response.version(_request.version());
	// ���ö�����
	_response.keep_alive(false);
	if (_request.method() == http::verb::get)
	{
		PreParseGetParam();
		bool success = LogicSystem::GetInstance()->HandleGet(_get_url, shared_from_this());
		if (!success)
		{
			_response.result(http::status::not_found);
			_response.set(http::field::content_type, "text/plain");
			beast::ostream(_response.body()) << "url not found\r\n";
			WriteResponse();
			return;
		}

		_response.result(http::status::ok);
		// ���������ط������ظ�������
		_response.set(http::field::server, "GateServer");
		WriteResponse();
		return;
	}

	if (_request.method() == http::verb::post)
	{
		bool success = LogicSystem::GetInstance()->HandlePost(_request.target(), shared_from_this());
		if (!success)
		{
			_response.result(http::status::not_found);
			_response.set(http::field::content_type, "text/plain");
			beast::ostream(_response.body()) << "url not found\r\n";
			WriteResponse();
			return;
		}

		_response.result(http::status::ok);
		// ���������ط������ظ�������
		_response.set(http::field::server, "GateServer");
		WriteResponse();
		return;
	}
}
