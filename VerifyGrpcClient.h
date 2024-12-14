#pragma once
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include "const.h"

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::GetVerifyReq;
using message::GetVerifyRsp;
using message::VerifyService;

class VerifyGrpcClient: public Singleton<VerifyGrpcClient>
{
	friend class Singleton<VerifyGrpcClient>;
public:
	GetVerifyRsp GetVerifyCode(std::string email)
	{
		ClientContext context;
		GetVerifyRsp reply;
		GetVerifyReq request;
		request.set_email(email);
		// 获取请求返回的状态
		Status status = stub_->GetVerifyCode(&context, request, &reply);
		if (status.ok())
		{
			return reply;
		}
		else
		{
			reply.set_error(ErrorCodes::RPCFailed);
			return reply;
		}
	}
private:
	VerifyGrpcClient()
	{
		// 第一个参数表示服务器的地址，第二个参数用于传递创建通道时需要的各种配置选项，grpc::InsecureChannelCredentials()用于创建一个不安全的通道凭据
		std::shared_ptr<Channel> channel = grpc::CreateChannel("0.0.0.0:50051", grpc::InsecureChannelCredentials());
		// stub必须通过channel通信
		stub_ = VerifyService::NewStub(channel);
	}
	// 通过stub才可以通信
	std::unique_ptr<VerifyService::Stub> stub_;
};

