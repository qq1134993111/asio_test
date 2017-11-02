#pragma once
#include <memory>
#include <mutex>
#include<unordered_map>


template <typename TSession>
class SessionManager
{
public:
	SessionManager() {};
	~SessionManager() {};
	bool Insert(std::shared_ptr<TSession> spsession);
	std::shared_ptr<TSession> Get(uint64_t sessionid);
	bool Remove(uint64_t sessionid);
private:
	std::mutex session_mutex_;
	std::unordered_map<uint64_t, std::weak_ptr<TSession>>  session_map_;
};

template <typename TSession>
inline bool SessionManager<TSession>::Insert(std::shared_ptr<TSession> spsession)
{
	std::unique_lock<std::mutex> lock1(session_mutex_);
	auto apair = session_map_.emplace(spsession->GetSessionID(), spsession);
	return apair.second;
}

template <typename TSession>
inline std::shared_ptr<TSession> SessionManager<TSession>::Get(uint64_t sessionid)
{
	std::unique_lock<std::mutex> lock1(session_mutex_);
	auto it = session_map_.find(sessionid);
	if (it != session_map_.end())
	{
		return it->second.lock();
	}

	return std::shared_ptr<TSession>();
}

template <typename TSession>
inline bool SessionManager<TSession>::Remove(uint64_t sessionid)
{
	std::unique_lock<std::mutex> lock1(session_mutex_);
	return session_map_.erase(sessionid);
}

