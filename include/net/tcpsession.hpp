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
class TcpServer;

template <typename TSession>
class TcpClient;


template <typename TSession>
class TcpSession : public std::enable_shared_from_this<TSession>, boost::noncopyable
{
public:
	TcpSession(boost::asio::io_service& ios, uint64_t sessionid, uint32_t check_recv_timeout_seconds = 0)
		:ios_(ios), socket_(ios_), sessionid_(sessionid), check_connect_delay_and_connect_timeout_and_heartbeat_timer_(ios_), check_recv_timeout_timer_(ios_), recv_timeout_seconds_(check_recv_timeout_seconds)
		, connect_delay_seconds_(0), heartbeat_intervals_seconds_(0), status_(SessionStatus::kInit)
	{


#ifdef SOCKET_HEADER_BODY_MODE

#else
		recv_buffer_.SetCapacitySize(4096);
#endif
	}
	~TcpSession();


public:

	bool Send(std::string data);
	std::deque<std::string> ClearSendQueue();

	void SetRecvTimeOut(uint32_t check_recv_timeout_seconds);

	void SetHeartbeat(std::string strInfo, uint32_t check_heartbeat_timeout_seconds = 0);

	void Shutdown(const boost::asio::socket_base::shutdown_type& what = boost::asio::ip::tcp::socket::shutdown_both, bool post = false);

	bool IsConnect();

	boost::asio::ip::tcp::socket& GetSocket() { return socket_; }

	boost::asio::io_service& GetIoService() { return ios_; }

	uint64_t GetSessionID() { return sessionid_; }

	const boost::asio::ip::tcp::endpoint& GetLocalEndpoint() const { return local_endpoint_; }
	const boost::asio::ip::tcp::endpoint& GetRemoteEndpoint() const { return remote_endpoint_; }


protected:
	friend class TcpServer<TSession>;
	friend class TcpClient<TSession>;

	bool Connect(const std::string &ip, unsigned short port, uint32_t delay_seconds = 0, uint32_t connect_timeout_seconds = 0);
	bool Connect(boost::asio::ip::tcp::endpoint & connect_endpoint, uint32_t delay_seconds = 0, uint32_t connect_timeout_seconds = 0);

	bool Start();

	void SetConnectCallback(ConnectCallback<TSession> fnconnect);

	void SetConnectFailureCallback(ConnectFailureCallback<TSession> fnconnectfailure);

#ifdef SOCKET_HEADER_BODY_MODE
	void SetMessageLengthCallback(HeaderLengthCallback fnheaderlength, BodyLengthCallback<TSession> fnbodylength);
	void SetMessageCallback(MessageCallback<TSession>  fnmessage);

#else
	void SetRecvCallback(RecvCallback<TSession> fnrecv);
#endif // SOCKET_HEADER_BODY_MODE

	void SetCloseCallback(CloseCallback<TSession> fnclose);


protected:
	void SetSocketNoDelay();

	void DoSetHeartbeat(std::string strInfo, uint32_t check_heartbeat_timeout_seconds = 0);

	void DoConnect(boost::asio::ip::tcp::endpoint & endpoint);
	void HandleConnect(const boost::system::error_code & ec);

	void HandleWrite(const boost::system::error_code & ec);

	void ExpiresRecvTimer();
	void CancelRecvTimer();
	void HandleRecvTimer(boost::system::error_code const& error);

	void DoShutdown(const boost::asio::socket_base::shutdown_type& what = boost::asio::ip::tcp::socket::shutdown_both, const boost::system::error_code& ec = boost::asio::error::operation_aborted);

	void ExpiresConnectDelayTimer();
	void ExpiresConnectTimeoutTimer();
	void ExpiresHeartbeatTimer();
	void CancelConnectDelayAndConnectTimeoutAndHeartbeatTimer();
	void HandleConnectDelayTimer(boost::system::error_code const & ec);
	void HandleConnectTimeoutTimer(boost::system::error_code const & ec);
	void HandleHeartbeatTimer(boost::system::error_code const & ec);

#ifdef SOCKET_HEADER_BODY_MODE
	void ReadHeader();
	void ReadBody();
	void HandleReadHeader(const boost::system::error_code & ec);
	void HandleReadBody(const boost::system::error_code & ec);
#else
	void ReadSome();
	void HandleReadSome(const boost::system::error_code & ec, std::size_t bytes_transferred);
#endif // SOCKET_HEADER_BODY_MODE

protected:
	enum class SessionStatus :uint8_t
	{
		kInit = 0,
		kConnecting,
		kRunning,
		kShuttingdown
	};

	enum class HandleResult :uint32_t
	{
		kHandleSuccess = 0,
		kHandleContinue,
		kHandlePause,
		kHandleError
	};
private:
	boost::asio::io_service& ios_;

	boost::asio::ip::tcp::socket socket_;
	uint64_t sessionid_;


	boost::asio::ip::tcp::endpoint local_endpoint_;
	boost::asio::ip::tcp::endpoint remote_endpoint_;

	boost::asio::steady_timer	check_connect_delay_and_connect_timeout_and_heartbeat_timer_;
	std::atomic<uint32_t>		connect_delay_seconds_;
	std::atomic<uint32_t>       connect_timeout_seconds_;
	std::atomic<uint32_t>		heartbeat_intervals_seconds_;
	std::string  send_heartbeat_info_;

	std::atomic<SessionStatus>  status_;



	ConnectFailureCallback<TSession> fnconnectfailure_;
	ConnectCallback<TSession>  fnconnect_;

#ifdef SOCKET_HEADER_BODY_MODE
	std::vector<uint8_t> header_;
	uint32_t header_size_;
	std::vector<uint8_t> body_;
	BodyLengthCallback<TSession> fnbodylength_;
	MessageCallback<TSession>  fnmessage_;
#else
	DataBuffer  recv_buffer_;
	RecvCallback<TSession>   fnrecv_;
#endif // SOCKET_HEADER_BODY_MODE

	std::deque<std::string> deq_messages_;
	std::mutex send_mtx_;


	boost::asio::steady_timer	check_recv_timeout_timer_;
	std::atomic<uint32_t>		recv_timeout_seconds_;

	CloseCallback<TSession> fnclose_;
};

template <typename TSession>
std::deque<std::string> TcpSession<TSession>::ClearSendQueue()
{
	std::unique_lock<std::mutex>  lc(send_mtx_);
	if (!IsConnect())
	{
		return std::move(deq_messages_);
	}

	return {};
}





template <typename TSession>
void TcpSession<TSession>::HandleHeartbeatTimer(boost::system::error_code const & ec)
{
	//if (ec == boost::asio::error::operation_aborted) /*重设/取消 */
	//{
	//	return;
	//}

	printf("FILE:%s,FUNCTION:%s,LINE:%d, %d,%s\n", __FILE__, __FUNCTION__, __LINE__, ec.value(), boost::system::system_error(ec).what());

	if (!ec)//0 操作成功
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
void TcpSession<TSession>::HandleConnectDelayTimer(boost::system::error_code const & ec)
{
	//if (ec == boost::asio::error::operation_aborted) /*重设/取消 */
	//{
	//	return;
	//}


	printf("FILE:%s,FUNCTION:%s,LINE:%d,%d,%s\n", __FILE__, __FUNCTION__, __LINE__, ec.value(), boost::system::system_error(ec).what());

	if (!ec)//0 操作成功
	{
		DoConnect(remote_endpoint_);
	}
}



template <typename TSession>
void TcpSession<TSession>::HandleConnectTimeoutTimer(boost::system::error_code const & ec)
{
	printf("FILE:%s,FUNCTION:%s,LINE:%d,%d,%s\n", __FILE__, __FUNCTION__, __LINE__, ec.value(), boost::system::system_error(ec).what());

	if (!ec)//0 操作成功
	{
		boost::system::error_code	ignored_ec;
		socket_.cancel(ignored_ec);
	}
}

template <typename TSession>
void TcpSession<TSession>::CancelConnectDelayAndConnectTimeoutAndHeartbeatTimer()
{
	boost::system::error_code	ignored_ec;
	size_t				size = check_connect_delay_and_connect_timeout_and_heartbeat_timer_.cancel(ignored_ec);

	printf("FILE:%s,FUNCTION:%s,LINE:%d, %d canceled,%d,%s\n", __FILE__, __FUNCTION__, __LINE__, size, ignored_ec.value(), boost::system::system_error(ignored_ec).what());

}

template <typename TSession>
void TcpSession<TSession>::ExpiresHeartbeatTimer()
{

	if (IsConnect())
	{
		if (heartbeat_intervals_seconds_ == 0)
			return;

		printf("FILE:%s,FUNCTION:%s,LINE:%d,heartbeat_intervals_seconds_:%d\n", __FILE__, __FUNCTION__, __LINE__, heartbeat_intervals_seconds_.load());

		check_connect_delay_and_connect_timeout_and_heartbeat_timer_.expires_from_now(std::chrono::seconds(heartbeat_intervals_seconds_));
		check_connect_delay_and_connect_timeout_and_heartbeat_timer_.async_wait(boost::bind(&TcpSession::HandleHeartbeatTimer, this->shared_from_this(), boost::asio::placeholders::error));
	}

}

template <typename TSession>
void TcpSession<TSession>::ExpiresConnectDelayTimer()
{
	if (connect_delay_seconds_ == 0)
		return;

	printf("FILE:%s,FUNCTION:%s,LINE:%d,connect_delay_seconds_:%d\n", __FILE__, __FUNCTION__, __LINE__, connect_delay_seconds_.load());

	check_connect_delay_and_connect_timeout_and_heartbeat_timer_.expires_from_now(std::chrono::seconds(connect_delay_seconds_));
	check_connect_delay_and_connect_timeout_and_heartbeat_timer_.async_wait(boost::bind(&TcpSession::HandleConnectDelayTimer, this->shared_from_this(), boost::asio::placeholders::error));

}

template <typename TSession>
void TcpSession<TSession>::ExpiresConnectTimeoutTimer()
{
	if (connect_timeout_seconds_ == 0)
		return;

	printf("FILE:%s,FUNCTION:%s,LINE:%d,connect_timeout_seconds_:%d\n", __FILE__, __FUNCTION__, __LINE__, connect_timeout_seconds_.load());

	check_connect_delay_and_connect_timeout_and_heartbeat_timer_.expires_from_now(std::chrono::seconds(connect_timeout_seconds_));
	check_connect_delay_and_connect_timeout_and_heartbeat_timer_.async_wait(boost::bind(&TcpSession::HandleConnectTimeoutTimer, this->shared_from_this(), boost::asio::placeholders::error));
}


template <typename TSession>
void TcpSession<TSession>::HandleConnect(const boost::system::error_code & ec)
{
	if (!ec)
	{
		Start();
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
bool TcpSession<TSession>::Connect(boost::asio::ip::tcp::endpoint & connect_endpoint, uint32_t delay_seconds /*= 0*/, uint32_t connect_timeout_seconds /*= 0*/)
{
	status_ = SessionStatus::kConnecting;

	auto self(this->shared_from_this());

	auto func = [&, self]() {

		if (socket_.is_open())
		{
			boost::system::error_code	ignored_ec;
			socket_.close(ignored_ec);
		}

		self->remote_endpoint_ = connect_endpoint;
		self->connect_delay_seconds_ = delay_seconds;
		self->connect_timeout_seconds_ = connect_timeout_seconds;

		printf("FILE:%s,FUNCTION:%s,LINE:%d,%s:%d,delay_seconds:%d\n", __FILE__, __FUNCTION__, __LINE__, connect_endpoint.address().to_string().c_str(), connect_endpoint.port(), delay_seconds);

		if (self->connect_delay_seconds_ != 0)
		{
			self->ExpiresConnectDelayTimer();
		}
		else
		{
			self->DoConnect(connect_endpoint);
		}

	};

	ios_.dispatch(func);

	return true;
}

template <typename TSession>
bool TcpSession<TSession>::Connect(const std::string &ip, unsigned short port, uint32_t delay_seconds /*= 0*/, uint32_t connect_timeout_seconds /*= 0*/)
{
	boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(ip), port);

	return Connect(endpoint, delay_seconds, connect_timeout_seconds);
}

template <typename TSession>
void TcpSession<TSession>::DoConnect(boost::asio::ip::tcp::endpoint & connect_endpoint)
{
	printf("FILE:%s,FUNCTION:%s,LINE:%d,%s:%d\n", __FILE__, __FUNCTION__, __LINE__, connect_endpoint.address().to_string().c_str(), connect_endpoint.port());

	socket_.async_connect(connect_endpoint,
		boost::bind(&TcpSession::HandleConnect, this->shared_from_this(), boost::asio::placeholders::error));


	ExpiresConnectTimeoutTimer();


}

template <typename TSession>
bool TcpSession<TSession>::IsConnect()
{
	return (status_ == SessionStatus::kRunning);
}

template <typename TSession>
void TcpSession<TSession>::Shutdown(const boost::asio::socket_base::shutdown_type& what, bool post)
{
	if (post)
	{
		ios_.post(boost::bind(&TcpSession::DoShutdown, std::enable_shared_from_this<TSession>::shared_from_this(), what, boost::asio::error::operation_aborted));
	}
	else
	{
		ios_.dispatch(boost::bind(&TcpSession::DoShutdown, std::enable_shared_from_this<TSession>::shared_from_this(), what, boost::asio::error::operation_aborted));
	}
}

template <typename TSession>
void TcpSession<TSession>::DoSetHeartbeat(std::string strInfo, uint32_t heartbeat_intervals_seconds /*= 0*/)
{
	if (IsConnect())
	{
		send_heartbeat_info_ = std::move(strInfo);
		heartbeat_intervals_seconds_ = heartbeat_intervals_seconds;
		if (heartbeat_intervals_seconds != 0)
		{
			ExpiresHeartbeatTimer();
		}
		else
		{
			CancelConnectDelayAndConnectTimeoutAndHeartbeatTimer();
		}
	}
}

template <typename TSession>
void TcpSession<TSession>::SetHeartbeat(std::string strInfo, uint32_t check_heartbeat_timeout_seconds /*= 0*/)
{
	ios_.post(boost::bind(&TcpSession::DoSetHeartbeat, this->shared_from_this(), std::move(strInfo), check_heartbeat_timeout_seconds));
}

template <typename TSession>
void TcpSession<TSession>::SetRecvTimeOut(uint32_t check_recv_timeout_seconds)
{
	auto self(this->shared_from_this());

	auto func = [self, check_recv_timeout_seconds]() {
		if (self->IsConnect())
		{
			self->recv_timeout_seconds_ = check_recv_timeout_seconds;

			if (self->recv_timeout_seconds_ == 0)
			{
				self->CancelRecvTimer();
			}

			self->ExpiresRecvTimer();
		}
	};

	ios_.post(func);

}

template <typename TSession>
bool TcpSession<TSession>::Send(std::string data)
{
	//ios_.post(boost::bind(&TcpSession::DoWrite, this->shared_from_this(), std::move(data)));

	std::unique_lock<std::mutex>  lc(send_mtx_);

	if (IsConnect())
	{
		bool write_in_progress = !deq_messages_.empty();
		deq_messages_.push_back(std::move(data));

		if (!write_in_progress)
		{
			boost::asio::async_write(socket_,
				boost::asio::buffer(deq_messages_.front()),
				boost::bind(&TcpSession::HandleWrite, std::enable_shared_from_this<TSession>::shared_from_this(), boost::asio::placeholders::error));
		}

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

	try
	{
		SetSocketNoDelay();
		local_endpoint_ = socket_.local_endpoint();
		remote_endpoint_ = socket_.remote_endpoint();

		CancelConnectDelayAndConnectTimeoutAndHeartbeatTimer();
	}
	catch (std::exception& e)
	{
		printf("%s,%d,%s\n", __FUNCTION__, __LINE__, e.what());

		boost::system::error_code	ignored_ec;
		socket_.close(ignored_ec);
		return false;
	}

	if (1)
	{
		auto self = this->shared_from_this();
		ios_.post([self]() {

			try
			{
				if (self->IsConnect())
				{
					assert(self->fnconnect_ != nullptr);
					self->fnconnect_(self);

#ifdef SOCKET_HEADER_BODY_MODE
					self->ReadHeader();
					self->ExpiresHeartbeatTimer();
#else
					self->ReadSome();
					self->ExpiresHeartbeatTimer();
#endif

			}
		}
			catch (std::exception& e)
			{
				printf("%s,%d,%s\n", __FUNCTION__, __LINE__, e.what());

				self->DoShutdown();
			}

	});
}

	//	try
	//	{
	//		assert(fnconnect_ != nullptr);
	//		fnconnect_(this->shared_from_this());
	//
	//		auto self = this->shared_from_this();
	//
	//#ifdef SOCKET_HEADER_BODY_MODE
	//
	//		ios_.post([self]() {
	//
	//			if (self->IsConnect())
	//			{
	//				self->ReadHeader();
	//				self->ExpiresHeartbeatTimer();
	//			}
	//		});
	//
	//
	//#else
	//
	//		ios_.post([self]() {
	//
	//			if (self->IsConnect())
	//			{
	//				self->ReadSome();
	//				self->ExpiresHeartbeatTimer();
	//			}
	//	});
	//
	//#endif // SOCKET_HEADER_BODY_MODE
	//
	//}
	//	catch (std::exception& e)
	//	{
	//		printf("%s,%d,%s\n", __FUNCTION__, __LINE__, e.what());
	//		DoShutdown();
	//		return false;
	//	}
	//


	return true;
}



#ifdef SOCKET_HEADER_BODY_MODE

template <typename TSession>
void TcpSession<TSession>::SetMessageCallback(MessageCallback<TSession> fnmessage)
{
	fnmessage_ = std::move(fnmessage);
}

template <typename TSession>
void TcpSession<TSession>::SetMessageLengthCallback(HeaderLengthCallback fnheaderlength, BodyLengthCallback<TSession> fnbodylength)
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
	boost::asio::async_read(socket_, boost::asio::buffer(body_), boost::asio::transfer_at_least(body_.size()),
		boost::bind(&TcpSession::HandleReadBody, std::enable_shared_from_this<TSession>::shared_from_this(), boost::asio::placeholders::error));

	ExpiresRecvTimer();

}

template <typename TSession>
void TcpSession<TSession>::HandleReadHeader(const boost::system::error_code & ec)
{
	if (!ec)
	{
		assert(fnbodylength_ != nullptr);

		int32_t size = fnbodylength_(std::enable_shared_from_this<TSession>::shared_from_this(), header_);
		if (size >= 0)
		{
			body_.resize(size);
			ReadBody();
		}
		else
		{
			DoShutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		}
	}
	else
	{
		//printf("%s,%d,%d,%s\n", __FUNCTION__, __LINE__, ec.value(), boost::system::system_error(ec).what());

		DoShutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
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
		DoShutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
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
		auto r = fnrecv_(this->shared_from_this(), recv_buffer_);
		if (r == (uint32_t)HandleResult::kHandleSuccess || r == (uint32_t)HandleResult::kHandleContinue)
		{
			ReadSome();
		}
	}
	else
	{
		//printf("%s,%d,%d,%s\n", __FUNCTION__, __LINE__, ec.value(), boost::system::system_error(ec).what());

		DoShutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
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

#endif // SOCKET_HEADER_BODY_MODE




template <typename TSession>
void TcpSession<TSession>::HandleWrite(const boost::system::error_code & ec)
{

	if (!ec)
	{
		std::unique_lock<std::mutex>  lc(send_mtx_);


		deq_messages_.pop_front();

		if (!deq_messages_.empty() && IsConnect())
		{
			boost::asio::async_write(socket_,
				boost::asio::buffer(deq_messages_.front()),
				boost::bind(&TcpSession::HandleWrite, std::enable_shared_from_this<TSession>::shared_from_this(), boost::asio::placeholders::error));
		}



	}
	else
	{

		//printf("%s,%d,%d,%s\n", __FUNCTION__, __LINE__, ec.value(), boost::system::system_error(ec).what());

		DoShutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
	}
}

template <typename TSession>
void TcpSession<TSession>::ExpiresRecvTimer()
{
	if (IsConnect())
	{
		if (recv_timeout_seconds_ == 0)
			return;

		printf("FILE:%s,FUNCTION:%s,LINE:%d,check_recv_timeout_seconds_:%d\n", __FILE__, __FUNCTION__, __LINE__, recv_timeout_seconds_.load());

		check_recv_timeout_timer_.expires_from_now(std::chrono::seconds(recv_timeout_seconds_));
		check_recv_timeout_timer_.async_wait(boost::bind(&TcpSession::HandleRecvTimer, this->shared_from_this(), boost::asio::placeholders::error));
	}

}

template <typename TSession>
void TcpSession<TSession>::CancelRecvTimer()
{
	boost::system::error_code	ignored_ec;
	size_t				size = check_recv_timeout_timer_.cancel(ignored_ec);

	printf("FILE:%s,FUNCTION:%s,LINE:%d, %d canceled,%d,%s\n", __FILE__, __FUNCTION__, __LINE__, size, ignored_ec.value(), boost::system::system_error(ignored_ec).what());
}

template <typename TSession>
void TcpSession<TSession>::HandleRecvTimer(boost::system::error_code const & ec)
{
	//if (ec == boost::asio::error::operation_aborted) /*重设/取消 */
	//{
	//	return;
	//}

	if (!ec)//0 操作成功
	{

		//printf("%s,%d,%d,%s\n", __FUNCTION__, __LINE__, error.value(), boost::system::system_error(error).what());

		//boost::system::error_code ec = boost::asio::error::timed_out;
		//printf("%s,%d,%d,%s\n", __FUNCTION__, __LINE__, ec.value(), boost::system::system_error(ec).what());

		DoShutdown(boost::asio::ip::tcp::socket::shutdown_both, boost::asio::error::timed_out);
	}
}

template <typename TSession>
void TcpSession<TSession>::DoShutdown(const boost::asio::socket_base::shutdown_type& what, const boost::system::error_code& ec)
{
	std::unique_lock<std::mutex>  lc(send_mtx_);

	if (IsConnect())
	{
		status_ = SessionStatus::kShuttingdown;

		CancelConnectDelayAndConnectTimeoutAndHeartbeatTimer();
		CancelRecvTimer();


		boost::system::error_code	ignored_ec;
		socket_.shutdown(/*boost::asio::ip::tcp::socket::shutdown_both*/what, ignored_ec);

		//socket_.close(ignored_ec);

		lc.unlock();

		assert(fnclose_ != nullptr);
		fnclose_(std::enable_shared_from_this<TSession>::shared_from_this(), ec);
	}
}