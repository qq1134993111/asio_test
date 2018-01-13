// server_test.cpp: 定义控制台应用程序的入口点。
//


#include "net/tcpserver.hpp"


class EchoSession : public TcpSession<EchoSession>
{
public:
	EchoSession(boost::asio::io_service& ios, uint64_t sessionid, uint32_t check_recv_timeout_seconds = 0) :
		TcpSession(ios, sessionid, check_recv_timeout_seconds)
	{

	}
	~EchoSession() {}
};

class MyServer :public TcpServer<EchoSession>
{
public:
	MyServer(uint16_t port, size_t pool_size) :TcpServer(port, pool_size) {}
	~MyServer() {};

public:
	virtual uint32_t OnGetHeaderLength()
	{
		return 10;
	};
	virtual int32_t OnGetBodyLength(std::shared_ptr<EchoSession> spsession,std::vector<uint8_t>& header)
	{
		return 100;
	}
	virtual void OnConnect(std::shared_ptr<EchoSession> spsession)
	{
		TcpServer::OnConnect(spsession);
		spsession->SetRecvTimeOut(10, true);
	};
	virtual int32_t OnMessage(std::shared_ptr<EchoSession> spsession, std::vector<uint8_t>& header, std::vector<uint8_t>& body)
	{
		//printf("header:%s\n", std::string(header.begin(), header.end()).c_str());
		//printf("body:%s\n", std::string(body.begin(), body.end()).c_str());
		//printf(" Speed: %d\n", spsession->GetRealTimeSpeed());
		static uint64_t i = 0;
		if (i++ > 100000000)
		{
			spsession->Shutdown();
			//spsession->Close();
		}
		else
		{
			spsession->Send(std::string(header.begin(), header.end()));
			spsession->Send(std::string(body.begin(), body.end()));
			//this->TcpSend(spsession->GetSessionID(), std::string(header.begin(), header.end()));
			//this->TcpSend(spsession->GetSessionID(), std::string(body.begin(), body.end()));
		}

		return 0;
	}

	virtual uint32_t OnRecv(std::shared_ptr<EchoSession> spsession, DataBuffer& recv_data)
	{
		spsession->Send(std::string((char*)recv_data.GetReadPtr(), recv_data.GetDataSize()));
		recv_data.Read(nullptr, recv_data.GetDataSize());
		return 0;
	}
	virtual void OnClose(std::shared_ptr<EchoSession> spsession, boost::system::error_code const& ec)
	{
		TcpServer::OnClose(spsession, ec);

		//printf("%s,%d,%d,%s\n", __FUNCTION__, __LINE__, ec.value(), boost::system::system_error(ec).what());
	};

	virtual void OnAcceptFailed(boost::system::error_code const& ec)
	{
		printf("FILE:%s,FUNCTION:%s,LINE:%d,ErrorCode:%d,ErrorString:%s\n", __FILE__, __FUNCTION__, __LINE__,
			ec.value(), boost::system::system_error(ec).what());
	}
};


int main()
{

	try
	{
		MyServer svr(8088, std::thread::hardware_concurrency());
		svr.Start();
		while (svr.IsRunning())
		{
			//Sleep(1000);
			std::chrono::milliseconds dura(2000);
			std::this_thread::sleep_for(dura);
		}
	}
	catch (std::exception&  e)
	{

		printf("server Error: %s\n", e.what());
	}

	return 0;
}
