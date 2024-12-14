#pragma once

#include "const.h"

// Ϊ�˷�ֹ��HttpConnection�໥���ã�������ͷ�ļ�����������Դ�ļ�������
class HttpConnection;

// HttpHandler ��һ�������������ͣ������Դ洢�����û����κο���ƥ����ǩ���ĺ�����lambda ���ʽ������������Ա����ָ��
typedef std::function<void(std::shared_ptr<HttpConnection>)> HttpHandler;

class LogicSystem: public Singleton<LogicSystem>
{
	// ��������ģ��������Ϊ��Ԫ��Ϊ�˿��Է���LogicSystem�Ĺ��캯��
	friend class Singleton<LogicSystem>;
public:
	~LogicSystem() {};
	// ����get����
	bool HandleGet(std::string, std::shared_ptr<HttpConnection>);
	// ע��get����
	void RegGet(std::string, HttpHandler handler);
	// ����post����
	bool HandlePost(std::string, std::shared_ptr<HttpConnection>);
	// ע��post����
	void RegPost(std::string, HttpHandler handler);
private:
	LogicSystem();
	std::map<std::string, HttpHandler> _post_handlers;
	std::map<std::string, HttpHandler> _get_handlers;
};

