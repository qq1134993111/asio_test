#pragma once
#include <thread>
#include <atomic>
#include "tcpsession.hpp"
#include "sessionmanager.hpp"

template <typename TSession>
class TcpClient :boost::noncopyable
{
public:
	TcpClient() :ios_(), work_(/*std::make_unique<boost::asio::io_service::work>(ios_)*/new boost::asio::io_service::work(ios_)), id_(0)
	{
		is_running_ = false;
	}
	~TcpClient() {};

	virtual void OnConnectFailure(std::shared_ptr<TSession> spsession, boost::system::error_code const& ec)
	{
		printf("FILE:%s,FUNCTION:%s,LINE:%d,SessionID:%lld,Local:%s:%d,Remote:%s:%d,ErrorCode:%d,ErrorString:%s\n", __FILE__, __FUNCTION__, __LINE__, spsession->GetSessionID(), \
			spsession->GetLocalEndpoint().address().to_string().c_str(), spsession->GetLocalEndpoint().port(),
			spsession->GetRemoteEndpoint().address().to_string().c_str(), spsession->GetRemoteEndpoint().port(),
			ec.value(), boost::system::system_error(ec).what());
	};

	virtual void OnConnect(std::shared_ptr<TSession> spsession)
	{
		session_mng_.Insert(spsession);

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

#ifdef 	  SOCKET_HEADER_BODY_MODE
	virtual uint32_t OnGetHeaderLength() = 0;
	virtual int32_t OnGetBodyLength(std::shared_ptr<TSession> spsession, std::vector<uint8_t>& header) = 0;
	virtual int32_t OnMessage(std::shared_ptr<TSession> spsession, std::vector<uint8_t>& header, std::vector<uint8_t>& body) = 0;
#else
	virtual uint32_t OnRecv(std::shared_ptr<TSession> spsession, DataBuffer& recv_data) = 0;
#endif // 

public:
	std::shared_ptr<TSession> Connect(std::string ip, uint16_t port, uint32_t delay_seconds = 0, uint32_t connect_timeout_seconds = 0)
	{
		auto new_session = std::make_shared<TSession>(ios_, ++id_);


		new_session->SetConnectCallback(std::bind(&TcpClient::OnConnect, this, std::placeholders::_1));

		new_session->SetConnectFailureCallback(std::bind(&TcpClient::OnConnectFailure, this, std::placeholders::_1, std::placeholders::_2));

#if SOCKET_HEADER_BODY_MODE
		new_session->SetMessageLengthCallback(std::bind(&TcpClient::OnGetHeaderLength, this), std::bind(&TcpClient::OnGetBodyLength, this, std::placeholders::_1, std::placeholders::_2));
		new_session->SetMessageCallback(std::bind(&TcpClient::OnMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
#else
		new_session->SetRecvCallback(std::bind(&TcpClient::OnRecv, this, std::placeholders::_1, std::placeholders::_2));
#endif

		new_session->SetCloseCallback(std::bind(&TcpClient::OnClose, this, std::placeholders::_1, std::placeholders::_2));

		new_session->Connect(ip, port, delay_seconds, connect_timeout_seconds);

		return new_session;

	}

	bool Run()
	{
		if (!is_running_)
		{
			if (ios_.stopped())
			{
				ios_.reset();
			}

			thread_run_ = std::thread([this]() {

				boost::system::error_code ec;
				this->ios_.run(ec);

			});

			is_running_ = true;
		}

		return is_running_;
	}

	void Stop()
	{
		work_.reset(nullptr);

		if (!ios_.stopped())
			ios_.stop();

		if (thread_run_.joinable())
			thread_run_.join();

		is_running_ = false;
	}

	bool IsRunning()
	{
		return is_running_;
	}

protected:
	/*void Connect(std::shared_ptr<TSession> spsession, std::string ip, uint16_t port, uint32_t delay_seconds = 0, uint32_t connect_timeout_seconds = 0)
	{
		spsession->Connect(ip, port, delay_seconds, connect_timeout_seconds);
	}

	void Connect(std::shared_ptr<TSession> spsession, boost::asio::ip::tcp::endpoint connect_endpoint, uint32_t delay_seconds = 0, uint32_t connect_timeout_seconds = 0)
	{
		spsession->Connect(connect_endpoint, delay_seconds, connect_timeout_seconds);
	}*/

protected:
	boost::asio::io_service ios_;
	std::unique_ptr<boost::asio::io_service::work>	work_;
	std::thread thread_run_;
	std::atomic<uint64_t> id_;
	SessionManager<TSession> session_mng_;
	bool is_running_;
};