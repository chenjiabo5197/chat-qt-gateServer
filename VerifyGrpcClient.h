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
			// 第一个参数表示服务器的地址，第二个参数用于传递创建通道时需要的各种配置选项，grpc::InsecureChannelCredentials()用于创建一个不安全的通道凭据
			std::shared_ptr<Channel> channel = grpc::CreateChannel(host + ":" + port, grpc::InsecureChannelCredentials());
			// stub必须通过channel通信
			connections_.push(VerifyService::NewStub(channel));
		}
	}

	~RPCConnPool()
	{
		// 加锁，std::lock_guard 是一个模板类，用于管理互斥锁（mutex）的锁定和解锁，确保互斥锁在作用域结束时自动释放
		// 会立即尝试锁定 mutex_,如果互斥锁已经被其他线程锁定，则当前线程会阻塞，直到互斥锁被释放并被当前线程成功锁定
		// std::lock_guard 的一个重要特性是，当 lock 对象被销毁时（通常是因为它离开了其作用域），它的析构函数会自动调用 mutex_ 的 unlock 方法来释放锁
		std::lock_guard<std::mutex> lock(mutex_);
		Close();
		while (!connections_.empty())
		{
			connections_.pop();
		}
	}

	// 通知所有线程，连接池被回收了
	void Close()
	{
		b_stop_ = true;
		cond_.notify_all();
	}

	// 从连接池返回一个链接
	std::unique_ptr<VerifyService::Stub> getConnection()
	{
		std::unique_lock<std::mutex> lock(mutex_);
		// wait方法会等待直到b_stop_变为true或者connections_不再为空。当其中一个条件满足时，线程会继续执行后面的代码
		cond_.wait(lock, [this]() {
			if (b_stop_)
			{
				// 继续往下走，且加锁
				return true;
			}
			// 线程会阻塞并等待条件变量的通知
			return !connections_.empty();
		});
		if (b_stop_)
		{
			return nullptr;
		}
		// 此时队列不为空
		auto context = std::move(connections_.front());
		connections_.pop();
		return context;
	}

	// 归还连接
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
	// std::condition_variable 是一个同步原语，它允许线程在某些条件满足时被通知或等待某个事件的发生
	std::condition_variable cond_;
	// std::mutex 是一个互斥锁类，用于保护共享数据，防止多个线程同时访问造成数据竞争
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
		// 获取请求返回的状态
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
	//	// 第一个参数表示服务器的地址，第二个参数用于传递创建通道时需要的各种配置选项，grpc::InsecureChannelCredentials()用于创建一个不安全的通道凭据
	//	std::shared_ptr<Channel> channel = grpc::CreateChannel("127.0.0.1:50051", grpc::InsecureChannelCredentials());
	//	// stub必须通过channel通信
	//	stub_ = VerifyService::NewStub(channel);
	//}
	//// 通过stub才可以通信
	//std::unique_ptr<VerifyService::Stub> stub_;
	VerifyGrpcClient();

	std::unique_ptr<RPCConnPool> pool_;
};

