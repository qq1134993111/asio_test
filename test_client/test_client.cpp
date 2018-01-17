
#include "net/tcpclient.hpp"

class EchoSession : public TcpSession<EchoSession>
{
public:
	EchoSession(boost::asio::io_service& ios, uint64_t sessionid, uint32_t check_recv_timeout_seconds = 0) :
		TcpSession(ios, sessionid, check_recv_timeout_seconds)
	{

	}
	~EchoSession()
	{
		printf("%s\n", __FUNCTION__);
	}

};

class MyClient :public TcpClient<EchoSession>
{
public:
	MyClient() :TcpClient() {}
	~MyClient() {};

	virtual void OnConnectFailure(std::shared_ptr<EchoSession> spsession, boost::system::error_code const& ec)
	{
		TcpClient::OnConnectFailure(spsession, ec);

		//Connect(spsession, spsession->GetRemoteEndpoint().address().to_string(), spsession->GetRemoteEndpoint().port(), 10, 1);
		auto endpoint = spsession->GetRemoteEndpoint();
		Connect(endpoint.address().to_string(), endpoint.port(), 10,1);
	};

	virtual void OnConnect(std::shared_ptr<EchoSession> spsession)
	{
		TcpClient::OnConnect(spsession);

		spsession->SetRecvTimeOut(15);
		spsession->SetHeartbeat("Heartbeat", 3);
	};
	virtual void OnClose(std::shared_ptr<EchoSession> spsession, boost::system::error_code const& ec)
	{
		TcpClient::OnClose(spsession, ec);

		//spsession->ClearSendQueue();
		//Connect(spsession, spsession->GetRemoteEndpoint(), 10);
		auto endpoint = spsession->GetRemoteEndpoint();
		Connect(endpoint.address().to_string(),endpoint.port(), 10,1);
	};

	virtual uint32_t OnRecv(std::shared_ptr<EchoSession> spsession, DataBuffer& recv_data)
	{
		std::string str(recv_data.GetDataSize(), '\0');
		recv_data.Read(&str[0], str.size());

		printf("FILE:%s,FUNCTION:%s,LINE:%d,SessionID:%lld,Local:%s:%d,Remote:%s:%d\n%s\n", __FILE__, __FUNCTION__, __LINE__, spsession->GetSessionID(), \
			spsession->GetLocalEndpoint().address().to_string().c_str(), spsession->GetLocalEndpoint().port(),
			spsession->GetRemoteEndpoint().address().to_string().c_str(), spsession->GetRemoteEndpoint().port(),
			str.c_str()
		);
		return 0;
	}

	virtual uint32_t OnGetHeaderLength()
	{
		return 10;
	};
	virtual int32_t OnGetBodyLength(std::shared_ptr<EchoSession> spsession, std::vector<uint8_t>& header)
	{
		return 10;
	}

	virtual int32_t OnMessage(std::shared_ptr<EchoSession> spsession, std::vector<uint8_t>& header, std::vector<uint8_t>& body)
	{
		printf("FILE:%s,FUNCTION:%s,LINE:%d,SessionID:%lld,Local:%s:%d,Remote:%s:%d,%s%s\n", __FILE__, __FUNCTION__, __LINE__, spsession->GetSessionID(), \
			spsession->GetLocalEndpoint().address().to_string().c_str(), spsession->GetLocalEndpoint().port(),
			spsession->GetRemoteEndpoint().address().to_string().c_str(), spsession->GetRemoteEndpoint().port(),
			header.data(), body.data()
		);

		return 0;
	}
};



int main()
{
	MyClient client;
	client.Run();

	client.Connect("127.0.0.1", 8088,10,3);

	//client.Connect("10.172.35.10", 8088, 10, 1);
	while (client.IsRunning())
	{
		std::chrono::milliseconds dura(2000);
		std::this_thread::sleep_for(dura);
	}

}
