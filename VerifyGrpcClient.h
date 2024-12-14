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
		// ��ȡ���󷵻ص�״̬
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
		// ��һ��������ʾ�������ĵ�ַ���ڶ����������ڴ��ݴ���ͨ��ʱ��Ҫ�ĸ�������ѡ�grpc::InsecureChannelCredentials()���ڴ���һ������ȫ��ͨ��ƾ��
		std::shared_ptr<Channel> channel = grpc::CreateChannel("0.0.0.0:50051", grpc::InsecureChannelCredentials());
		// stub����ͨ��channelͨ��
		stub_ = VerifyService::NewStub(channel);
	}
	// ͨ��stub�ſ���ͨ��
	std::unique_ptr<VerifyService::Stub> stub_;
};

