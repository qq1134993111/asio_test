#pragma once

#include <memory>
#include <functional>
#include <vector>
#include "boost/filesystem.hpp"
#include "buffer/databuffer.hpp"


template <typename TSession>
using  TcpSessionPtr = std::shared_ptr<TSession>;

template <typename TSession>
using  ConnectCallback = std::function<void(TcpSessionPtr<TSession> session_ptr)>;

template <typename TSession>
using  ConnectFailureCallback = std::function<void(TcpSessionPtr<TSession> session_ptr, const boost::system::error_code& ec)>;

template <typename TSession>
using CloseCallback = std::function<void(TcpSessionPtr<TSession> session_ptr, const boost::system::error_code& ec)>;

template <typename TSession>
using RecvCallback = std::function<uint32_t(TcpSessionPtr<TSession> session_ptr, DataBuffer& recv_data)>;

using HeaderLengthCallback = std::function<uint32_t()>;

template <typename TSession>
using BodyLengthCallback = std::function<int32_t(TcpSessionPtr<TSession> session_ptr,std::vector<uint8_t>& header)>;

template <typename TSession>
using MessageCallback = std::function<void(TcpSessionPtr<TSession> session_ptr, std::vector<uint8_t>& header, std::vector<uint8_t>& body)>;

