#include "VerifyGrpcClient.h"
#include "ConfigMgr.h"

VerifyGrpcClient::VerifyGrpcClient()
{
	auto& gCfgMgr = ConfigMgr::Inst();
	std::string host = gCfgMgr["VerifyServer"]["Host"];
	std::string port = gCfgMgr["VerifyServer"]["Port"];
	pool_.reset(new RPCConnPool(5, host, port));
}
