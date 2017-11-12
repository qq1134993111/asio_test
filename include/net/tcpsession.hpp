#pragma once
#include <memory>
#include <stdint.h>
#include <string>
#include <mutex>
#include <atomic>

#include "boost/noncopyable.hpp"
#include "boost/asio.hpp"
#include "boost/bind.hpp"
#include "boost/asio/steady_timer.hpp"
#include "boost/chrono.hpp"
#include "tcpsessioncallback.h"

template <typename TSession>
class SessionManager;

template <typename TSession>
class TcpSession : public std::enable_shared_from_this<TSession>, boost::noncopyable
{
public:
	TcpSession(boost::asio::io_service& ios, uint64_t sessionid, uint32_t check_recv_timeout_seconds = 0)
		:ios_(ios), socket_(ios_), sessionid_(sessionid), check_connect_delay_and_heartbeat_timer_(ios_), check_recv_timer_(ios_), check_recv_timeout_seconds_(check_recv_timeout_seconds)
		, check_connect_delay_seconds_(0), check_heartbeat_timeout_seconds_(0), status_(SessionStatus::kInit)
	{
#ifdef SERVER_HEADER_BODY_MODE

#else
		recv_buffer_.SetCapacitySize(4096);
#endif
	}
	~TcpSession();


public:

	void SetConnectCallback(ConnectCallback<TSession> fnconnect);

	void SetConnectFailureCallback(ConnectFailureCallback<TSession> fnconnectfailure);

#ifdef SERVER_HEADER_BODY_MODE
	void SetMessageLengthCallback(HeaderLengthCallback fnheaderlength, BodyLengthCallback fnbodylength);
	void SetMessageCallback(MessageCallback<TSession>  fnmessage);

#else
	void SetRecvCallback(RecvCallback<TSession> fnrecv);
#endif // SERVER_HEADER_BODY_MODE

	void SetCloseCallback(CloseCallback<TSession> fnclose);

	bool Start();

	bool Send(std::string data);

	void SetRecvTimeOut(uint32_t check_recv_timeout_seconds, bool immediately = false);

	void SetHeartbeat(std::string strInfo, uint32_t check_heartbeat_timeout_seconds = 0);



	void Shutdown();

	bool IsConnect();

	boost::asio::ip::tcp::socket& GetSocket() { return socket_; }

	boost::asio::io_service& GetIoService() { return ios_; }

	uint64_t GetSessionID() { return sessionid_; }

	const boost::asio::ip::tcp::endpoint& GetLocalEndpoint() const { return local_endpoint_; }
	const boost::asio::ip::tcp::endpoint& GetRemoteEndpoint() const { return remote_endpoint_; }


	bool Connect(const std::string &ip, unsigned short port, uint32_t delay_seconds = 0);

	bool Connect(boost::asio::ip::tcp::endpoint & connect_endpoint, uint32_t delay_seconds = 0);

protected:
	void SetSocketNoDelay();
	void DoSetHeartbeat(std::string strInfo, uint32_t check_heartbeat_timeout_seconds = 0);

	void DoConnect(boost::asio::ip::tcp::endpoint & endpoint);
	void HandleConnect(const boost::system::error_code & ec);

	void DoWrite(std::string  data);
	void HandleWrite(const boost::system::error_code & ec);

	void ExpiresRecvTimer();
	void CancelRecvTimer();

	void HandleRecvTimeOut(boost::system::error_code const& error);

	void DoShutdown(const boost::system::error_code& ec = boost::asio::error::eof);

	void ExpiresConnectDelayTimer();

	void ExpiresHeartbeatTimer();


	void CancelConnectDelayAndHeartbeatTimer();

	void HandleConnectDelayTimeOut(boost::system::error_code const & ec);

	void HandleHeartbeatTimeOut(boost::system::error_code const & ec);


#ifdef SERVER_HEADER_BODY_MODE
	void ReadHeader();
	void ReadBody();
	void HandleReadHeader(const boost::system::error_code & ec);
	void HandleReadBody(const boost::system::error_code & ec);
#else
	void ReadSome();
	void HandleReadSome(const boost::system::error_code & ec, std::size_t bytes_transferred);
#endif // SERVER_HEADER_BODY_MODE

protected:
	enum class SessionStatus :uint8_t
	{
		kInit = 0,
		kConnecting,
		kRunning,
		kShuttingdown
	};

private:
	boost::asio::io_service& ios_;

	boost::asio::ip::tcp::socket socket_;
	uint64_t sessionid_;


	boost::asio::ip::tcp::endpoint local_endpoint_;
	boost::asio::ip::tcp::endpoint remote_endpoint_;

	boost::asio::steady_timer	check_connect_delay_and_heartbeat_timer_;
	std::atomic<uint32_t>		check_connect_delay_seconds_;
	std::atomic<uint32_t>		check_heartbeat_timeout_seconds_;
	std::string  send_heartbeat_info_;

	std::atomic<SessionStatus>  status_;



	ConnectFailureCallback<TSession> fnconnectfailure_;
	ConnectCallback<TSession>  fnconnect_;

#ifdef SERVER_HEADER_BODY_MODE
	std::vector<uint8_t> header_;
	uint32_t header_size_;
	std::vector<uint8_t> body_;
	BodyLengthCallback fnbodylength_;
	MessageCallback<TSession>  fnmessage_;
#else
	DataBuffer  recv_buffer_;
	RecvCallback<TSession>   fnrecv_;
#endif // SERVER_HEADER_BODY_MODE

	std::deque<std::string> deq_messages_;
	std::mutex send_mtx_;

	boost::asio::steady_timer	check_recv_timer_;
	std::atomic<uint32_t>		check_recv_timeout_seconds_;

	CloseCallback<TSession> fnclose_;
};

template <typename TSession>
void TcpSession<TSession>::HandleHeartbeatTimeOut(boost::system::error_code const & ec)
{
	//if (ec == boost::asio::error::operation_aborted) /*����/ȡ�� */
	//{
	//	return;
	//}

	//if (!socket_.is_open())
	//	return;
	printf("HandleHeartbeatTimeOut:%d,%s\n", ec.value(), boost::system::system_error(ec).what());
	if (!ec)//0 �����ɹ�
	{
		if (1)
		{
			std::unique_lock<std::mutex>  lc(send_mtx_);

			if (deq_messages_.empty())
			{
				deq_messages_.push_back(send_heartbeat_info_);

				boost::asio::async_write(socket_,
					boost::asio::buffer(deq_messages_.front()),
					boost::bind(&TcpSession::HandleWrite, std::enable_shared_from_this<TSession>::shared_from_this(), boost::asio::placeholders::error));
			}
		}

		ExpiresHeartbeatTimer();
	}
}

template <typename TSession>
void TcpSession<TSession>::HandleConnectDelayTimeOut(boost::system::error_code const & ec)
{
	//if (ec == boost::asio::error::operation_aborted) /*����/ȡ�� */
	//{
	//	return;
	//}

	//if (!socket_.is_open())
	//	return;
	printf("HandleConnectDelayTimeOut:%d,%s\n", ec.value(), boost::system::system_error(ec).what());
	if (!ec)//0 �����ɹ�
	{
		DoConnect(remote_endpoint_);
	}
}

template <typename TSession>
void TcpSession<TSession>::CancelConnectDelayAndHeartbeatTimer()
{
	boost::system::error_code	ignored_ec;
	size_t				size = check_connect_delay_and_heartbeat_timer_.cancel(ignored_ec);

	printf("FILE:%s,FUNCTION:%s,LINE:%d, %d canceled,%d,%s\n", __FILE__, __FUNCTION__, __LINE__, size, ignored_ec.value(), boost::system::system_error(ignored_ec).what());

}

template <typename TSession>
void TcpSession<TSession>::ExpiresHeartbeatTimer()
{

	if (IsConnect())
	{
		if (check_heartbeat_timeout_seconds_ == 0)
			return;

		printf("FILE:%s,FUNCTION:%s,LINE:%d,check_heartbeat_timeout_seconds_:%d\n", __FILE__, __FUNCTION__, __LINE__, check_heartbeat_timeout_seconds_.load());

		check_connect_delay_and_heartbeat_timer_.expires_from_now(std::chrono::seconds(check_heartbeat_timeout_seconds_));
		check_connect_delay_and_heartbeat_timer_.async_wait(boost::bind(&TcpSession::HandleHeartbeatTimeOut, this->shared_from_this(), boost::asio::placeholders::error));
	}

}

template <typename TSession>
void TcpSession<TSession>::ExpiresConnectDelayTimer()
{
	if (socket_.is_open())
	{
		socket_.close();
	}

	if (check_connect_delay_seconds_ == 0)
		return;

	printf("FILE:%s,FUNCTION:%s,LINE:%d,check_connect_delay_seconds_:%d\n", __FILE__, __FUNCTION__, __LINE__, check_connect_delay_seconds_.load());

	check_connect_delay_and_heartbeat_timer_.expires_from_now(std::chrono::seconds(check_connect_delay_seconds_));
	check_connect_delay_and_heartbeat_timer_.async_wait(boost::bind(&TcpSession::HandleConnectDelayTimeOut, this->shared_from_this(), boost::asio::placeholders::error));

}



template <typename TSession>
void TcpSession<TSession>::HandleConnect(const boost::system::error_code & ec)
{
	if (!ec)
	{
		Start();
		assert(fnconnect_ != nullptr);
		fnconnect_(this->shared_from_this());
	}
	else
	{
		//error
		assert(fnconnectfailure_ != nullptr);
		fnconnectfailure_(this->shared_from_this(), ec);
	}
}

template <typename TSession>
void TcpSession<TSession>::SetSocketNoDelay()
{
	boost::asio::ip::tcp::no_delay option(true);
	boost::system::error_code ec;
	socket_.set_option(option, ec);
}

template <typename TSession>
bool TcpSession<TSession>::Connect(boost::asio::ip::tcp::endpoint & connect_endpoint, uint32_t delay_seconds /*= 0*/)
{
	status_ = SessionStatus::kConnecting;

	remote_endpoint_ = connect_endpoint;
	check_connect_delay_seconds_ = delay_seconds;

	printf("FILE:%s,FUNCTION:%s,LINE:%d,%s:%d,delay_seconds:%d\n", __FILE__, __FUNCTION__, __LINE__, connect_endpoint.address().to_string().c_str(), connect_endpoint.port(), delay_seconds);

	if (check_connect_delay_seconds_ != 0)
	{
		ExpiresConnectDelayTimer();
	}
	else
	{
		DoConnect(connect_endpoint);
	}

	return true;
}

template <typename TSession>
bool TcpSession<TSession>::Connect(const std::string &ip, unsigned short port, uint32_t delay_seconds /*= 0*/)
{
	boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(ip), port);

	return Connect(endpoint, delay_seconds);
}

template <typename TSession>
void TcpSession<TSession>::DoConnect(boost::asio::ip::tcp::endpoint & connect_endpoint)
{
	printf("FILE:%s,FUNCTION:%s,LINE:%d,%s:%d\n", __FILE__, __FUNCTION__, __LINE__, connect_endpoint.address().to_string().c_str(), connect_endpoint.port());

	socket_.async_connect(connect_endpoint,
		boost::bind(&TcpSession::HandleConnect, this->shared_from_this(), boost::asio::placeholders::error));
}

template <typename TSession>
bool TcpSession<TSession>::IsConnect()
{
	return (status_ == SessionStatus::kRunning);
}

template <typename TSession>
void TcpSession<TSession>::Shutdown()
{
	ios_.dispatch(boost::bind(&TcpSession::DoShutdown, std::enable_shared_from_this<TSession>::shared_from_this(), boost::asio::error::operation_aborted));
}

template <typename TSession>
void TcpSession<TSession>::DoSetHeartbeat(std::string strInfo, uint32_t check_heartbeat_timeout_seconds /*= 0*/)
{
	send_heartbeat_info_ = std::move(strInfo);
	check_heartbeat_timeout_seconds_ = check_heartbeat_timeout_seconds;
	if (check_heartbeat_timeout_seconds != 0)
	{
		ExpiresHeartbeatTimer();
	}
	else
	{
		CancelConnectDelayAndHeartbeatTimer();
	}
}

template <typename TSession>
void TcpSession<TSession>::SetHeartbeat(std::string strInfo, uint32_t check_heartbeat_timeout_seconds /*= 0*/)
{
	ios_.post(boost::bind(&TcpSession::DoSetHeartbeat, this->shared_from_this(), std::move(strInfo), check_heartbeat_timeout_seconds));
}

template <typename TSession>
void TcpSession<TSession>::SetRecvTimeOut(uint32_t check_recv_timeout_seconds, bool immediately /*= false*/)
{
	check_recv_timeout_seconds_ = check_recv_timeout_seconds;

	if (check_recv_timeout_seconds_ == 0)
	{
		CancelRecvTimer();
	}

	if (immediately)
	{
		ExpiresRecvTimer();
	}
}

template <typename TSession>
bool TcpSession<TSession>::Send(std::string data)
{
	if (IsConnect())
	{
		//ios_.post(boost::bind(&TcpSession::DoWrite, this->shared_from_this(), std::move(data)));

		DoWrite(std::move(data));

		return true;
	}

	return false;
}

template <typename TSession>
void TcpSession<TSession>::SetCloseCallback(CloseCallback<TSession> fnclose)
{
	fnclose_ = std::move(fnclose);
}





template <typename TSession>
void TcpSession<TSession>::SetConnectFailureCallback(ConnectFailureCallback<TSession> fnconnectfailure)
{
	fnconnectfailure_ = std::move(fnconnectfailure);
}

template <typename TSession>
void TcpSession<TSession>::SetConnectCallback(ConnectCallback<TSession> fnconnect)
{
	fnconnect_ = std::move(fnconnect);
}

template <typename TSession>
TcpSession<TSession>::~TcpSession()
{
	boost::system::error_code	ignored_ec;
	socket_.close(ignored_ec);

	printf("%s,%d,%d,%s\n", __FUNCTION__, __LINE__, ignored_ec.value(), boost::system::system_error(ignored_ec).what());

}

template <typename TSession>
bool TcpSession<TSession>::Start()
{

	status_ = SessionStatus::kRunning;

	SetSocketNoDelay();
	local_endpoint_ = socket_.local_endpoint();
	remote_endpoint_ = socket_.remote_endpoint();

	ExpiresHeartbeatTimer();

#ifdef SERVER_HEADER_BODY_MODE
	ReadHeader();
#else

	ReadSome();
#endif // SERVER_HEADER_BODY_MODE


	return true;
}


template <typename TSession>
void TcpSession<TSession>::DoWrite(std::string  data)
{
	std::unique_lock<std::mutex>  lc(send_mtx_);

	bool write_in_progress = !deq_messages_.empty();
	deq_messages_.push_back(std::move(data));

	if (!write_in_progress)
	{
		if (IsConnect())
		{
			boost::asio::async_write(socket_,
				boost::asio::buffer(deq_messages_.front()),
				boost::bind(&TcpSession::HandleWrite, std::enable_shared_from_this<TSession>::shared_from_this(), boost::asio::placeholders::error));
		}
	}

}

#ifdef SERVER_HEADER_BODY_MODE

template <typename TSession>
void TcpSession<TSession>::SetMessageCallback(MessageCallback<TSession> fnmessage)
{
	fnmessage_ = std::move(fnmessage);
}

template <typename TSession>
void TcpSession<TSession>::SetMessageLengthCallback(HeaderLengthCallback fnheaderlength, BodyLengthCallback fnbodylength)
{
	header_size_ = fnheaderlength();
	fnbodylength_ = std::move(fnbodylength);
}

template <typename TSession>
void TcpSession<TSession>::ReadHeader()
{
	header_.resize(header_size_);

	boost::asio::async_read(socket_, boost::asio::buffer(header_), boost::asio::transfer_at_least(header_size_),
		boost::bind(&TcpSession::HandleReadHeader, this->shared_from_this(), boost::asio::placeholders::error));


	ExpiresRecvTimer();

}

template <typename TSession>
void TcpSession<TSession>::ReadBody()
{

	assert(fnbodylength_ != nullptr);

	size_t size = fnbodylength_(header_);
	body_.resize(size);

	boost::asio::async_read(socket_, boost::asio::buffer(body_), boost::asio::transfer_at_least(size),
		boost::bind(&TcpSession::HandleReadBody, std::enable_shared_from_this<TSession>::shared_from_this(), boost::asio::placeholders::error));

	ExpiresRecvTimer();

}

template <typename TSession>
void TcpSession<TSession>::HandleReadHeader(const boost::system::error_code & ec)
{
	if (!ec)
	{
		ReadBody();
	}
	else
	{
		//printf("%s,%d,%d,%s\n", __FUNCTION__, __LINE__, ec.value(), boost::system::system_error(ec).what());

		DoShutdown(ec);
	}
}

template<typename TSession>
void TcpSession<TSession>::HandleReadBody(const boost::system::error_code & ec)
{

	if (!ec)
	{
		assert(fnmessage_);
		fnmessage_(std::enable_shared_from_this<TSession>::shared_from_this(), header_, body_);

		ReadHeader();
	}
	else
	{
		//printf("%s,%d,%d,%s\n", __FUNCTION__, __LINE__, ec.value(), boost::system::system_error(ec).what());
		DoShutdown(ec);
	}
}


#else


template <typename TSession>
void TcpSession<TSession>::SetRecvCallback(RecvCallback<TSession> fnrecv)
{
	fnrecv_ = std::move(fnrecv);
}


template <typename TSession>
void TcpSession<TSession>::HandleReadSome(const boost::system::error_code & ec, std::size_t bytes_transferred)
{
	if (!ec)
	{
		recv_buffer_.SetWritePos(recv_buffer_.GetWritePos() + bytes_transferred);
		fnrecv_(this->shared_from_this(), recv_buffer_);
		ReadSome();
	}
	else
	{
		//printf("%s,%d,%d,%s\n", __FUNCTION__, __LINE__, ec.value(), boost::system::system_error(ec).what());

		DoShutdown(ec);
	}
}

template <typename TSession>
void TcpSession<TSession>::ReadSome()
{
	recv_buffer_.Adjustment();

	socket_.async_read_some(boost::asio::buffer(recv_buffer_.GetWritePtr(), recv_buffer_.GetAvailableSize()),
		boost::bind(&TcpSession::HandleReadSome, this->shared_from_this(),
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));

	ExpiresRecvTimer();
	}

#endif // SERVER_HEADER_BODY_MODE




template <typename TSession>
void TcpSession<TSession>::HandleWrite(const boost::system::error_code & ec)
{

	if (!ec)
	{
		std::unique_lock<std::mutex>  lc(send_mtx_);

		deq_messages_.pop_front();

		if (!deq_messages_.empty())
		{
			boost::asio::async_write(socket_,
				boost::asio::buffer(deq_messages_.front()),
				boost::bind(&TcpSession::HandleWrite, std::enable_shared_from_this<TSession>::shared_from_this(), boost::asio::placeholders::error));
		}

	}
	else
	{

		//printf("%s,%d,%d,%s\n", __FUNCTION__, __LINE__, ec.value(), boost::system::system_error(ec).what());

		DoShutdown(ec);
	}
}

template <typename TSession>
void TcpSession<TSession>::ExpiresRecvTimer()
{
	if (IsConnect())
	{
		if (check_recv_timeout_seconds_ == 0)
			return;

		printf("FILE:%s,FUNCTION:%s,LINE:%d,check_recv_timeout_seconds_:%d\n", __FILE__, __FUNCTION__, __LINE__, check_recv_timeout_seconds_.load());

		check_recv_timer_.expires_from_now(std::chrono::seconds(check_recv_timeout_seconds_));
		check_recv_timer_.async_wait(boost::bind(&TcpSession::HandleRecvTimeOut, this->shared_from_this(), boost::asio::placeholders::error));
	}

}

template <typename TSession>
void TcpSession<TSession>::CancelRecvTimer()
{
	boost::system::error_code	ignored_ec;
	size_t				size = check_recv_timer_.cancel(ignored_ec);

	printf("FILE:%s,FUNCTION:%s,LINE:%d, %d canceled,%d,%s\n", __FILE__, __FUNCTION__, __LINE__, size, ignored_ec.value(), boost::system::system_error(ignored_ec).what());
}

template <typename TSession>
void TcpSession<TSession>::HandleRecvTimeOut(boost::system::error_code const & ec)
{
	//if (ec == boost::asio::error::operation_aborted) /*����/ȡ�� */
	//{
	//	return;
	//}

	//if (!socket_.is_open())
	//	return;

	if (!ec)//0 �����ɹ�
	{

		//printf("%s,%d,%d,%s\n", __FUNCTION__, __LINE__, error.value(), boost::system::system_error(error).what());

		//boost::system::error_code ec = boost::asio::error::timed_out;
		//printf("%s,%d,%d,%s\n", __FUNCTION__, __LINE__, ec.value(), boost::system::system_error(ec).what());

		DoShutdown(boost::asio::error::timed_out);
	}
}

template <typename TSession>
void TcpSession<TSession>::DoShutdown(const boost::system::error_code& ec)
{

	if (status_ != SessionStatus::kShuttingdown)
	{
		status_ = SessionStatus::kShuttingdown;

		CancelConnectDelayAndHeartbeatTimer();
		CancelRecvTimer();

		boost::system::error_code	ignored_ec;
		socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);

		//socket_.close(ignored_ec);

		assert(fnclose_ != nullptr);

		fnclose_(std::enable_shared_from_this<TSession>::shared_from_this(), ec);
	}
}