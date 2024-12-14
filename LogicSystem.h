#pragma once

#include "const.h"

// 为了防止和HttpConnection类互引用，所以在头文件里声明，在源文件里引用
class HttpConnection;

// HttpHandler 是一个函数对象类型，它可以存储、调用或复制任何可以匹配其签名的函数、lambda 表达式、函数对象或成员函数指针
typedef std::function<void(std::shared_ptr<HttpConnection>)> HttpHandler;

class LogicSystem: public Singleton<LogicSystem>
{
	// 将单例的模板类声明为友元，为了可以访问LogicSystem的构造函数
	friend class Singleton<LogicSystem>;
public:
	~LogicSystem() {};
	// 处理get请求
	bool HandleGet(std::string, std::shared_ptr<HttpConnection>);
	// 注册get请求
	void RegGet(std::string, HttpHandler handler);
	// 处理post请求
	bool HandlePost(std::string, std::shared_ptr<HttpConnection>);
	// 注册post请求
	void RegPost(std::string, HttpHandler handler);
private:
	LogicSystem();
	std::map<std::string, HttpHandler> _post_handlers;
	std::map<std::string, HttpHandler> _get_handlers;
};

