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
	TcpSession(boost::asio::io_service& ios, uint64_t sessionid, uint64_t check_recv_timeout_seconds = 0)
		:ios_(ios), socket_(ios_), sessionid_(sessionid), check_timer_(ios_), check_recv_timeout_seconds_(check_recv_timeout_seconds)
	{
#ifdef SERVER_HEADER_BODY_MODE

#else
		recv_buffer_.SetCapacitySize(4096);
#endif
	}
	~TcpSession();


public:
#ifdef SERVER_HEADER_BODY_MODE
	void SetMessageLengthCallback(HeaderLengthCallback fnheaderlength, BodyLengthCallback fnbodylength)
	{
		header_size_ = fnheaderlength();
		fnbodylength_ = std::move(fnbodylength);
	}
	void SetMessageCallback(MessageCallback<TSession>  fnmessage) { fnmessage_ = std::move(fnmessage); }

#else
	void SetRecvCallback(RecvCallback<TSession> fnrecv) { fnrecv_ = std::move(fnrecv); }
#endif // SERVER_HEADER_BODY_MODE

	void SetCloseCallback(CloseCallback<TSession> fnclose) { fnclose_ = std::move(fnclose); }

	bool Start();
	bool Send(std::string data)
	{
		if (!socket_.is_open())
			return false;

		std::unique_lock<decltype(send_mtx_)> lc(send_mtx_);
		bool write_in_progress = !deq_messages_.empty();
		deq_messages_.push_back(std::move(data));

		if (!write_in_progress)
		{
			if (socket_.is_open())
			{
				boost::asio::async_write(socket_,
					boost::asio::buffer(deq_messages_.front()),
					boost::bind(&TcpSession::HandleWrite, std::enable_shared_from_this<TSession>::shared_from_this(), boost::asio::placeholders::error));
			}

			/*
			auto func = [self = this->shared_from_this()]()
			{
				if (self->socket_.is_open())
				{
					boost::asio::async_write(self->socket_,
						boost::asio::buffer(self->deq_messages_.front()),
						boost::bind(&TcpSession::HandleWrite, self, boost::asio::placeholders::error));
				}

			};

			ios_.post(std::move(func));
			*/

			/*
			ios_.post(std::bind([](decltype(this->shared_from_this()) self)
			{

			if (self->socket_.is_open())
			{
			boost::asio::async_write(self->socket_,
			boost::asio::buffer(self->deq_messages_.front()),
			boost::bind(&TcpSession::HandleWrite, self, boost::asio::placeholders::error));
			}

			}, this->shared_from_this()));
			*/
		}
		//ios_.post(boost::bind(&TcpSession::DoWrite, this->shared_from_this(), std::move(data)));
		//DoWrite(std::move(data));
		return (true);
	}

	void SetRecvTimeOut(uint64_t check_recv_timeout_seconds)
	{
		check_recv_timeout_seconds_ = check_recv_timeout_seconds;
	}

	void DispatchClose()
	{
		ios_.dispatch(boost::bind(&TcpSession::DoDispatchClose, std::enable_shared_from_this<TSession>::shared_from_this()));
	}

	bool IsConnect()
	{
		return socket_.is_open();
	}


	boost::asio::ip::tcp::socket& GetSocket() { return socket_; }
	boost::asio::io_service& GetIoService() { return ios_; }
	uint64_t GetSessionID() { return sessionid_; }

	boost::asio::ip::tcp::endpoint GetLocalEndpoint() const { return local_endpoint_; }
	boost::asio::ip::tcp::endpoint GetRemoteEndpoint() const { return remote_endpoint_; }

protected:
	void SetNoDelay()
	{
		boost::asio::ip::tcp::no_delay option(true);
		boost::system::error_code ec;
		socket_.set_option(option, ec);
	}

#ifdef SERVER_HEADER_BODY_MODE
	void ReadHeader();
	void ReadBody();
	void HandleReadHeader(const boost::system::error_code & ec);
	void HandleReadBody(const boost::system::error_code & ec);
#else
	void ReadSome()
	{

		recv_buffer_.Adjustment();

		socket_.async_read_some(boost::asio::buffer(recv_buffer_.GetWritePtr(), recv_buffer_.GetAvailableSize()),
			boost::bind(&TcpSession::HandleReadSome, this->shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));

		ExpiresTimer();
	}

	void HandleReadSome(const boost::system::error_code & ec, std::size_t bytes_transferred)
	{
		if (!ec)
		{
			CancelTimer();
			recv_buffer_.SetWritePos(recv_buffer_.GetWritePos() + bytes_transferred);
			fnrecv_(this->shared_from_this(), recv_buffer_);
			ReadSome();
		}
		else
		{


			//printf("%s,%d,%d,%s\n", __FUNCTION__, __LINE__, ec.value(), boost::system::system_error(ec).what());

			Close();

		}
	}
#endif // SERVER_HEADER_BODY_MODE


	void DoWrite(std::string  data);
	void HandleWrite(const boost::system::error_code & ec);
	void ExpiresTimer();
	void CancelTimer();
	void HandleTimeOut(boost::system::error_code const& error);
	void Close(const boost::system::error_code& ec = boost::asio::error::eof);
	void DoDispatchClose()
	{
		boost::system::error_code const ec = boost::asio::error::operation_aborted; //本端关闭套接字
		Close(ec);
	};
private:
	boost::asio::io_service& ios_;

	boost::asio::ip::tcp::socket socket_;
	uint64_t sessionid_;


	boost::asio::ip::tcp::endpoint local_endpoint_;
	boost::asio::ip::tcp::endpoint remote_endpoint_;
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


	boost::asio::steady_timer	check_timer_;
	std::atomic<uint64_t>		check_recv_timeout_seconds_;

	CloseCallback<TSession> fnclose_;
};

template <typename TSession>
TcpSession<TSession>::~TcpSession()
{

}

template <typename TSession>
bool TcpSession<TSession>::Start()
{
	SetNoDelay();
	local_endpoint_ = socket_.local_endpoint();
	remote_endpoint_ = socket_.remote_endpoint();

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

	std::unique_lock<decltype(send_mtx_)> lc(send_mtx_);
	bool write_in_progress = !deq_messages_.empty();
	deq_messages_.push_back(std::move(data));

	if (!write_in_progress)
	{
		if (socket_.is_open())
		{
			boost::asio::async_write(socket_,
				boost::asio::buffer(deq_messages_.front()),
				boost::bind(&TcpSession::HandleWrite, std::enable_shared_from_this<TSession>::shared_from_this(), boost::asio::placeholders::error));
		}
		
		/*
		auto func = [self = this->shared_from_this()]()
		{
			if (self->socket_.is_open())
			{
				boost::asio::async_write(self->socket_,
					boost::asio::buffer(self->deq_messages_.front()),
					boost::bind(&TcpSession::HandleWrite, self, boost::asio::placeholders::error));
			}

		};

		ios_.post(std::move(func));
		*/

		/*
		ios_.post(std::bind([](decltype(this->shared_from_this()) self) 
		{
		
			if (self->socket_.is_open())
			{
				boost::asio::async_write(self->socket_,
					boost::asio::buffer(self->deq_messages_.front()),
					boost::bind(&TcpSession::HandleWrite, self, boost::asio::placeholders::error));
			}

		}, this->shared_from_this()));
		*/
	}

}

#ifdef SERVER_HEADER_BODY_MODE
template <typename TSession>
void TcpSession<TSession>::ReadHeader()
{
	header_.resize(header_size_);

	boost::asio::async_read(socket_, boost::asio::buffer(header_), boost::asio::transfer_at_least(header_size_),
		boost::bind(&TcpSession::HandleReadHeader, this->shared_from_this(), boost::asio::placeholders::error));


	ExpiresTimer();

}

template <typename TSession>
void TcpSession<TSession>::ReadBody()
{

	assert(fnbodylength_ != nullptr);

	size_t size = fnbodylength_(header_);
	body_.resize(size);

	boost::asio::async_read(socket_, boost::asio::buffer(body_), boost::asio::transfer_at_least(size),
		boost::bind(&TcpSession::HandleReadBody, std::enable_shared_from_this<TSession>::shared_from_this(), boost::asio::placeholders::error));

	ExpiresTimer();

}

template <typename TSession>
void TcpSession<TSession>::HandleReadHeader(const boost::system::error_code & ec)
{
	if (!ec)
	{
		CancelTimer();
		ReadBody();
	}
	else
	{
		//printf("%s,%d,%d,%s\n", __FUNCTION__, __LINE__, ec.value(), boost::system::system_error(ec).what());

		Close();
	}
}

template <typename TSession>
void TcpSession<TSession>::HandleReadBody(const boost::system::error_code & ec)
{

	if (!ec)
	{
		CancelTimer();

		assert(fnmessage_);
		fnmessage_(std::enable_shared_from_this<TSession>::shared_from_this(), header_, body_);

		ReadHeader();
	}
	else
	{
		//printf("%s,%d,%d,%s\n", __FUNCTION__, __LINE__, ec.value(), boost::system::system_error(ec).what());
		Close();
	}
}
#else

#endif // SERVER_HEADER_BODY_MODE




template <typename TSession>
void TcpSession<TSession>::HandleWrite(const boost::system::error_code & ec)
{

	if (!ec)
	{
		std::unique_lock<decltype(send_mtx_)> lc(send_mtx_);

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

		Close();
	}
}

template <typename TSession>
void TcpSession<TSession>::ExpiresTimer()
{
	if (check_recv_timeout_seconds_ == 0)
		return;

	check_timer_.expires_from_now(std::chrono::seconds(check_recv_timeout_seconds_));
	check_timer_.async_wait(boost::bind(&TcpSession::HandleTimeOut, this->shared_from_this(), boost::asio::placeholders::error));
}

template <typename TSession>
void TcpSession<TSession>::CancelTimer()
{
	if (check_recv_timeout_seconds_ == 0)
		return;

	boost::system::error_code	ignored_ec;
	size_t				size = check_timer_.cancel(ignored_ec);
	printf("check_timer_ %d canceled\n", size);
}

template <typename TSession>
void TcpSession<TSession>::HandleTimeOut(boost::system::error_code const & error)
{
	//if (ec == boost::asio::error::operation_aborted) /*重设/取消 */
	//{
	//	return;
	//}

	//if (!socket_.is_open())
	//	return;

	if (!error)
	{
		//printf("%s,%d,%d,%s\n", __FUNCTION__, __LINE__, error.value(), boost::system::system_error(error).what());

		boost::system::error_code ec = boost::asio::error::timed_out;
		//printf("%s,%d,%d,%s\n", __FUNCTION__, __LINE__, ec.value(), boost::system::system_error(ec).what());

		Close();
	}
}

template <typename TSession>
void TcpSession<TSession>::Close(const boost::system::error_code& ec)
{
	if (socket_.is_open())
	{
		boost::system::error_code	ignored_ec;
		socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);

		socket_.close(ignored_ec);

		assert(fnclose_ != nullptr);

		fnclose_(std::enable_shared_from_this<TSession>::shared_from_this(), ec);
	}
}