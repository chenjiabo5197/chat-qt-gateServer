#include "LogicSystem.h"
#include "HttpConnection.h"
#include "VerifyGrpcClient.h"
#include "RedisMgr.h"

bool LogicSystem::HandleGet(std::string path, std::shared_ptr<HttpConnection> conn)
{
	// 未找到请求对应的url
	if (_get_handlers.find(path) == _get_handlers.end())
	{
		return false;
	}
	_get_handlers[path](conn);
	return true;
}

void LogicSystem::RegGet(std::string url, HttpHandler handler)
{
	_get_handlers.insert(make_pair(url, handler));
}

bool LogicSystem::HandlePost(std::string path, std::shared_ptr<HttpConnection> conn)
{
	// 未找到请求对应的url
	if (_post_handlers.find(path) == _post_handlers.end())
	{
		return false;
	}
	_post_handlers[path](conn);
	return true;
}

void LogicSystem::RegPost(std::string url, HttpHandler handler)
{
	_post_handlers.insert(make_pair(url, handler));
}

LogicSystem::LogicSystem()
{
	RegGet("/get_test", [](std::shared_ptr<HttpConnection> connection) {
		beast::ostream(connection->_response.body()) << "receive get_test req";
		int i = 0;
		for (auto& elem : connection->_get_params)
		{
			i++;
			beast::ostream(connection->_response.body()) << " param " << i << " key=" << elem.first;
			beast::ostream(connection->_response.body()) << " , " << i << " value=" << elem.second << std::endl;
		}
	});

	// 获取验证码请求
	RegPost("/get_verifycode", [](std::shared_ptr<HttpConnection> connection) {
		// 获取post请求体
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body=" << body_str << std::endl;
		// 设置response类型
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		// 将body_str字符串json反序列化到src_root中
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success)
		{
			std::cout << "Failed to parse Json data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonStr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonStr;
			// 不能返回false，返回false是断开连接
			return true;
		}

		// 判断请求体中是否有email这个key值
		if (!src_root.isMember("email"))
		{
			std::cout << "Failed to parse Json data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonStr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonStr;
			return true;
		}
		auto email = src_root["email"].asString();
		// 获取验证码服务器返回的response
		GetVerifyRsp rsp = VerifyGrpcClient::GetInstance()->GetVerifyCode(email);
		std::cout << "email=" << email << std::endl;
		// 获取返回值的error可以直接用error()，这是grpc封装的借口
		root["error"] = rsp.error();
		root["email"] = src_root["email"];
		std::string jsonStr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonStr;
		return true;
	});

	// 用户注册请求
	RegPost("/user_register", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		auto email = src_root["email"].asString();
		auto name = src_root["name"].asString();
		auto passwd = src_root["passwd"].asString();
		auto confirm = src_root["confirm"].asString();
		// 判断密码和确认密码是否一致
		if (std::strcmp(passwd.c_str(), confirm.c_str()))
		{
			std::cout << "confirm error" << std::endl;
			root["error"] = ErrorCodes::PasswdErr;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		// 查找redis中email对应的验证码是否过期
		std::string varify_code;
		bool b_get_varify = RedisMgr::GetInstance()->Get(CODEPREFIX + src_root["email"].asString(), varify_code);
		if (!b_get_varify) {
			std::cout << " get varify code expired" << std::endl;
			root["error"] = ErrorCodes::VerifyExpired;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		// 对比redis中的验证码与请求的验证码是否一致
		if (varify_code != src_root["varifycode"].asString()) {
			std::cout << " varify code error" << std::endl;
			root["error"] = ErrorCodes::VerifyCodeErr;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		//查找数据库判断用户是否存在

		root["error"] = 0;
		root["email"] = email;
		root["user"] = name;
		root["passwd"] = passwd;
		root["confirm"] = confirm;
		root["varifycode"] = src_root["varifycode"].asString();
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
	});
}
