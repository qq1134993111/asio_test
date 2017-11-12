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

	virtual void OnRecv(std::shared_ptr<TSession> spsession, DataBuffer& recv_data) = 0;

public:
	std::shared_ptr<TSession> Connect(std::string ip, uint16_t port, uint32_t delay_seconds = 0)
	{
		auto new_session = std::make_shared<TSession>(ios_, ++id_);


		new_session->SetConnectCallback(std::bind(&TcpClient::OnConnect, this, std::placeholders::_1));

		new_session->SetConnectFailureCallback(std::bind(&TcpClient::OnConnectFailure, this, std::placeholders::_1, std::placeholders::_2));

		new_session->SetRecvCallback(std::bind(&TcpClient::OnRecv, this, std::placeholders::_1, std::placeholders::_2));
		new_session->SetCloseCallback(std::bind(&TcpClient::OnClose, this, std::placeholders::_1, std::placeholders::_2));

		new_session->Connect(ip, port, delay_seconds);

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
	boost::asio::io_service ios_;
	std::unique_ptr<boost::asio::io_service::work>	work_;
	std::thread thread_run_;
	std::atomic<uint64_t> id_;
	SessionManager<TSession> session_mng_;
	bool is_running_;
};