#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

#include "Include.h"

// https://stackoverflow.com/questions/15278343/c11-thread-safe-queue
class TaskBatch
{

public:
	TaskBatch(size_t numThreads);
	~TaskBatch();
	void EnqueueTask(std::function<void()> task);
	void ExecuteTasks();

private:
	std::vector<std::thread> m_Threads;
	std::queue<std::function<void()>> m_Tasks;
	mutable std::mutex m_Mutex;
	bool TaskBatch::TryDequeue(std::function<void()>* task);
};