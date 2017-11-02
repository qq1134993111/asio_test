#pragma once
#include <stdint.h>
#include <string>
#include <mutex>
#include <unordered_map>
#include "boost/noncopyable.hpp"
#include "boost/system/error_code.hpp"
#include "ioservicepool.hpp"
#include "tcpsession.hpp"
#include "sessionmanager.hpp"

template <typename TSession>
class TcpServer :boost::noncopyable
{
public:
	TcpServer(uint16_t port, size_t pool_size = std::thread::hardware_concurrency()) :ios_pool_(pool_size),
		acceptor_(ios_pool_.GetIoService(), boost::asio::ip::tcp::endpoint{ boost::asio::ip::tcp::v4(), port }), id_(0), is_running_(false) {};
	virtual ~TcpServer();
	void Start()
	{
		ios_pool_.Start();
		DoAccept();
		is_running_ = true;
	}

	bool IsRunning() { return is_running_; }

	void Stop()
	{
		boost::system::error_code ec;
		acceptor_.close(ec);

		ios_pool_.Stop();
		is_running_ = false;
	}

	bool TcpSend(uint64_t sessionid, const std::string& data)
	{
		auto session = session_mng_.Get(sessionid);
		if (session != NULL)
		{
			session->Send(data);
		}

		return false;
	}

#ifdef 	  SERVER_HEADER_BODY_MODE
	virtual uint32_t OnGetHeaderLength() = 0;
	virtual size_t OnGetBodyLength(const std::vector<uint8_t>& header) = 0;
	virtual void OnMessage(std::shared_ptr<TSession> spsession, const std::vector<uint8_t>& header, const std::vector<uint8_t>& body) = 0;
#else
	virtual void OnRecv(std::shared_ptr<TSession> spsession, DataBuffer& recv_data) = 0;
#endif // 

	virtual void OnAcceptFailed(boost::system::error_code const& ec)
	{
		printf("FILE:%s,FUNCTION:%s,LINE:%d,ErrorCode:%d,ErrorString:%s\n", __FILE__, __FUNCTION__, __LINE__,
			ec.value(), boost::system::system_error(ec).what());
	}


	virtual void OnConnect(std::shared_ptr<TSession> spsession)
	{
		printf("FILE:%s,FUNCTION:%s,LINE:%d,SessionID:%lld,Local:%s:%d,Remote:%s:%d\n", __FILE__, __FUNCTION__, __LINE__, spsession->GetSessionID(), \
			spsession->GetLocalEndpoint().address().to_string().c_str(), spsession->GetLocalEndpoint().port(),
			spsession->GetRemoteEndpoint().address().to_string().c_str(), spsession->GetRemoteEndpoint().port()
		);
	};
	virtual void OnClose(std::shared_ptr<TSession> spsession, boost::system::error_code const& ec)
	{
		printf("FILE:%s,FUNCTION:%s,LINE:%d,SessionID:%lld,Local:%s:%d,Remote:%s:%d,ErrorCode:%d,ErrorString:%s\n", __FILE__, __FUNCTION__, __LINE__, spsession->GetSessionID(), \
			spsession->GetLocalEndpoint().address().to_string().c_str(), spsession->GetLocalEndpoint().port(),
			spsession->GetRemoteEndpoint().address().to_string().c_str(), spsession->GetRemoteEndpoint().port(),
			ec.value(), boost::system::system_error(ec).what());

		session_mng_.Remove(spsession->GetSessionID());
	};

protected:
	void DoAccept();
	SessionManager<TSession> session_mng_;
private:
	IoServicePool				ios_pool_;
	boost::asio::ip::tcp::acceptor	acceptor_;
	uint64_t id_;
	bool is_running_;
};

#include <functional>
#include "tcpserver.hpp"

template <typename TSession>
TcpServer<TSession>::~TcpServer()
{
}

template <typename TSession>
void TcpServer<TSession >::DoAccept()
{
	auto new_session = std::make_shared<TSession>(ios_pool_.GetIoService(), ++id_);

	acceptor_.async_accept(new_session->GetSocket(),
		[this, new_session](boost::system::error_code const& error)
	{
		if (!error)
		{
			session_mng_.Insert(new_session);

#ifdef 	   SERVER_HEADER_BODY_MODE
			new_session->SetMessageLengthCallback(std::bind(&TcpServer::OnGetHeaderLength, this), std::bind(&TcpServer::OnGetBodyLength, this, std::placeholders::_1));
			new_session->SetMessageCallback(std::bind(&TcpServer::OnMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
#else
			new_session->SetRecvCallback(std::bind(&TcpServer::OnRecv, this, std::placeholders::_1, std::placeholders::_2));
#endif 
			new_session->SetCloseCallback(std::bind(&TcpServer::OnClose, this, std::placeholders::_1, std::placeholders::_2));

			new_session->Start();

			OnConnect(new_session);
		}
		else
		{
			// TODO log error
			OnAcceptFailed(error);
		}

		DoAccept();
	});
}