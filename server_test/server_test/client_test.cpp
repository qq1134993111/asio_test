
#include "tcpclient.hpp"

class EchoSession : public TcpSession<EchoSession>
{
public:
	EchoSession(boost::asio::io_service& ios, uint64_t sessionid, uint32_t check_recv_timeout_seconds = 0) :
		TcpSession(ios, sessionid, check_recv_timeout_seconds)
	{

	}
	~EchoSession() {}
};

class MyClient :public TcpClient<EchoSession>
{
public:
	MyClient() :TcpClient() {}
	~MyClient() {};

	virtual void OnConnectFailure(std::shared_ptr<EchoSession> spsession)
	{
		TcpClient::OnConnectFailure(spsession);

		spsession->Connect((boost::asio::ip::tcp::endpoint)spsession->GetRemoteEndpoint(), 10);

		/*printf("FILE:%s,FUNCTION:%s,LINE:%d,SessionID:%lld,Local:%s:%d,Remote:%s:%d\n", __FILE__, __FUNCTION__, __LINE__, spsession->GetSessionID(), \
			spsession->GetLocalEndpoint().address().to_string().c_str(), spsession->GetLocalEndpoint().port(),
			spsession->GetRemoteEndpoint().address().to_string().c_str(), spsession->GetRemoteEndpoint().port()
		);*/
	};

	virtual void OnConnect(std::shared_ptr<EchoSession> spsession)
	{
		TcpClient::OnConnect(spsession);

		spsession->SetHeartbeat("Heartbeat", 3);

		/*	printf("FILE:%s,FUNCTION:%s,LINE:%d,SessionID:%lld,Local:%s:%d,Remote:%s:%d\n", __FILE__, __FUNCTION__, __LINE__, spsession->GetSessionID(), \
				spsession->GetLocalEndpoint().address().to_string().c_str(), spsession->GetLocalEndpoint().port(),
				spsession->GetRemoteEndpoint().address().to_string().c_str(), spsession->GetRemoteEndpoint().port()
			);*/
	};
	virtual void OnClose(std::shared_ptr<EchoSession> spsession, boost::system::error_code const& ec)
	{
		TcpClient::OnClose(spsession, ec);

		/*	printf("FILE:%s,FUNCTION:%s,LINE:%d,SessionID:%lld,Local:%s:%d,Remote:%s:%d,ErrorCode:%d,ErrorString:%s\n", __FILE__, __FUNCTION__, __LINE__, spsession->GetSessionID(), \
				spsession->GetLocalEndpoint().address().to_string().c_str(), spsession->GetLocalEndpoint().port(),
				spsession->GetRemoteEndpoint().address().to_string().c_str(), spsession->GetRemoteEndpoint().port(),
				ec.value(), boost::system::system_error(ec).what());*/

		spsession->Connect((boost::asio::ip::tcp::endpoint)spsession->GetRemoteEndpoint(), 10);
	};

	virtual void OnRecv(std::shared_ptr<EchoSession> spsession, DataBuffer& recv_data)
	{
		std::string str(recv_data.GetDataSize(), '\0');
		recv_data.Read(&str[0], str.size());

		printf("FILE:%s,FUNCTION:%s,LINE:%d,SessionID:%lld,Local:%s:%d,Remote:%s:%d\n%s\n", __FILE__, __FUNCTION__, __LINE__, spsession->GetSessionID(), \
			spsession->GetLocalEndpoint().address().to_string().c_str(), spsession->GetLocalEndpoint().port(),
			spsession->GetRemoteEndpoint().address().to_string().c_str(), spsession->GetRemoteEndpoint().port(),
			str.c_str()
		);


	}
};



int main()
{
	MyClient client;
	client.Run();

	client.Connect("127.0.0.1", 8088);

	while (client.IsRunning())
	{
		std::chrono::milliseconds dura(2000);
		std::this_thread::sleep_for(dura);
	}

}
