#pragma once
#include <memory>
#include <thread>
#include <exception>
#include<list>
#include "boost/noncopyable.hpp"
#include "boost/asio.hpp"
#include "boost/thread.hpp"
#include "boost/system/error_code.hpp"

class IoServicePool : boost::noncopyable
{
public:
	using ios_work_ptr = std::unique_ptr<boost::asio::io_service::work>;
	class IosWorker
	{
	public:
		IosWorker(std::size_t concurrency_hint = 1)
			: ios_(concurrency_hint)
			, concurrency_hint_(concurrency_hint)
			, workers_()
			, work_(/*std::make_unique<boost::asio::io_service::work>(ios_)*/new boost::asio::io_service::work(ios_))
		{}

		void Start()
		{

			for (size_t i = 0; i < concurrency_hint_; i++)
			{
				workers_.emplace_back(std::bind(&IosWorker::Run, this));
			}

		}

		void Stop()
		{
			work_.reset();
			if (!ios_.stopped())
				ios_.stop();
		}

		void Wait()
		{
			for (auto &w : workers_)
			{
				if (w.joinable())
					w.join();
			}
		}

		boost::asio::io_service& GetIoService()
		{
			return ios_;
		}

	private:
		void Run()
		{
			boost::system::error_code ec;
			ios_.run(ec);
		};

		boost::asio::io_service ios_;
		size_t  concurrency_hint_;
		std::vector<std::thread>	workers_;
		ios_work_ptr	work_;

	};
	using iterator = std::list<IosWorker>::iterator;
public:
	explicit IoServicePool(size_t pool_size = std::thread::hardware_concurrency())
		: ios_workers_(pool_size)
		, next_io_service_(ios_workers_.begin())
	{
	}

	~IoServicePool()
	{
		Stop();
	}

	bool Start()
	{
		try
		{
			for (auto& ios_worker : ios_workers_)
				ios_worker.Start();
		}
		catch (std::exception& e)
		{
			(void)e;
			return false;
		}

		return true;
	}

	void Stop()
	{
		for (auto& ios : ios_workers_)
			ios.Stop();

		for (auto& ios : ios_workers_)
			ios.Wait();
	}

	boost::asio::io_service& GetIoService()
	{
		auto current = next_io_service_++;
		if (ios_workers_.end() == next_io_service_)
		{
			next_io_service_ = ios_workers_.begin();
		}

		return current->GetIoService();
	}

private:
	std::list<IosWorker>		ios_workers_;
	iterator					next_io_service_;
};

