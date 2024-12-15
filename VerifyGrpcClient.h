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

class RPCConnPool 
{
public:
	RPCConnPool(size_t poolsize, std::string host, std::string port): poolSize_(poolsize), host_(host), port_(port), b_stop_(false)
	{
		for (size_t i = 0; i < poolSize_; i++)
		{
			// ��һ��������ʾ�������ĵ�ַ���ڶ����������ڴ��ݴ���ͨ��ʱ��Ҫ�ĸ�������ѡ�grpc::InsecureChannelCredentials()���ڴ���һ������ȫ��ͨ��ƾ��
			std::shared_ptr<Channel> channel = grpc::CreateChannel(host + ":" + port, grpc::InsecureChannelCredentials());
			// stub����ͨ��channelͨ��
			connections_.push(VerifyService::NewStub(channel));
		}
	}

	~RPCConnPool()
	{
		// ������std::lock_guard ��һ��ģ���࣬���ڹ���������mutex���������ͽ�����ȷ�������������������ʱ�Զ��ͷ�
		// �������������� mutex_,����������Ѿ��������߳���������ǰ�̻߳�������ֱ�����������ͷŲ�����ǰ�̳߳ɹ�����
		// std::lock_guard ��һ����Ҫ�����ǣ��� lock ��������ʱ��ͨ������Ϊ���뿪���������򣩣����������������Զ����� mutex_ �� unlock �������ͷ���
		std::lock_guard<std::mutex> lock(mutex_);
		Close();
		while (!connections_.empty())
		{
			connections_.pop();
		}
	}

	// ֪ͨ�����̣߳����ӳر�������
	void Close()
	{
		b_stop_ = true;
		cond_.notify_all();
	}

	// �����ӳط���һ������
	std::unique_ptr<VerifyService::Stub> getConnection()
	{
		std::unique_lock<std::mutex> lock(mutex_);
		// wait������ȴ�ֱ��b_stop_��Ϊtrue����connections_����Ϊ�ա�������һ����������ʱ���̻߳����ִ�к���Ĵ���
		cond_.wait(lock, [this]() {
			if (b_stop_)
			{
				// ���������ߣ��Ҽ���
				return true;
			}
			// �̻߳��������ȴ�����������֪ͨ
			return !connections_.empty();
		});
		if (b_stop_)
		{
			return nullptr;
		}
		// ��ʱ���в�Ϊ��
		auto context = std::move(connections_.front());
		connections_.pop();
		return context;
	}

	// �黹����
	void returnConnection(std::unique_ptr<VerifyService::Stub> context)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (b_stop_)
		{
			return;
		}
		connections_.push(std::move(context));
		cond_.notify_one();
	}

private:
	std::atomic<bool> b_stop_;
	size_t poolSize_;
	std::string host_;
	std::string port_;
	std::queue<std::unique_ptr<VerifyService::Stub>> connections_;
	// std::condition_variable ��һ��ͬ��ԭ��������߳���ĳЩ��������ʱ��֪ͨ��ȴ�ĳ���¼��ķ���
	std::condition_variable cond_;
	// std::mutex ��һ���������࣬���ڱ����������ݣ���ֹ����߳�ͬʱ����������ݾ���
	std::mutex mutex_;
};

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
		auto stub = pool_->getConnection();
		// ��ȡ���󷵻ص�״̬
		Status status = stub->GetVerifyCode(&context, request, &reply);
		if (status.ok())
		{
			pool_->returnConnection(std::move(stub));
			return reply;
		}
		else
		{
			pool_->returnConnection(std::move(stub));
			reply.set_error(ErrorCodes::RPCFailed);
			return reply;
		}
	}
private:
	//VerifyGrpcClient()
	//{
	//	// ��һ��������ʾ�������ĵ�ַ���ڶ����������ڴ��ݴ���ͨ��ʱ��Ҫ�ĸ�������ѡ�grpc::InsecureChannelCredentials()���ڴ���һ������ȫ��ͨ��ƾ��
	//	std::shared_ptr<Channel> channel = grpc::CreateChannel("127.0.0.1:50051", grpc::InsecureChannelCredentials());
	//	// stub����ͨ��channelͨ��
	//	stub_ = VerifyService::NewStub(channel);
	//}
	//// ͨ��stub�ſ���ͨ��
	//std::unique_ptr<VerifyService::Stub> stub_;
	VerifyGrpcClient();

	std::unique_ptr<RPCConnPool> pool_;
};

