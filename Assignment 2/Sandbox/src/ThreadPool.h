#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

#include "Include.h"

// https://stackoverflow.com/questions/15278343/c11-thread-safe-queue
class ThreadPool
{

public:
	ThreadPool(size_t numThreads);
	~ThreadPool();
	void Enqueue(std::function<void()> task);
	void LaunchThreads();
	void WaitTillDone();

private:
	std::vector<std::thread> m_Threads;
	std::queue<std::function<void()>> m_Tasks;
	mutable std::mutex m_Mutex;
	bool ThreadPool::TryDequeue(std::function<void()>* task);
};