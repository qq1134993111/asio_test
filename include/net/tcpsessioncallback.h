#pragma once

#include <memory>
#include <functional>
#include <vector>
#include "boost/filesystem.hpp"
#include "buffer/databuffer.hpp"


template <typename TSession>
using  TcpSessionPtr = std::shared_ptr<TSession>;

template <typename TSession>
using  ConnectCallback = std::function<void(TcpSessionPtr<TSession> sesionptr)>;

template <typename TSession>
using  ConnectFailureCallback = std::function<void(TcpSessionPtr<TSession> sesionptr, const boost::system::error_code& ec)>;

template <typename TSession>
using CloseCallback = std::function<void(TcpSessionPtr<TSession> sesionptr, const boost::system::error_code& ec)>;

template <typename TSession>
using RecvCallback = std::function<uint32_t(TcpSessionPtr<TSession> sesionptr, DataBuffer& recv_data)>;

using HeaderLengthCallback = std::function<uint32_t()>;
using BodyLengthCallback = std::function<size_t(std::vector<uint8_t>& header)>;

template <typename TSession>
using MessageCallback = std::function<void(const TcpSessionPtr<TSession>& sesionptr, std::vector<uint8_t>& header, std::vector<uint8_t>& body)>;
using PackageLengthCallback = std::function< int32_t(uint8_t* pdata, uint32_t len)>;

