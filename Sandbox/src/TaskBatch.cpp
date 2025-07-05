#include "TaskBatch.h"

TaskBatch::TaskBatch(size_t numThreads)
{
	m_Threads.resize(numThreads);
}

TaskBatch::~TaskBatch()
{
}

void TaskBatch::EnqueueTask(std::function<void()> task)
{
	const std::lock_guard<std::mutex> lock(m_Mutex);
	m_Tasks.push(task);
}


void TaskBatch::ExecuteTasks()
{
	for (size_t i = 0; i < m_Threads.size(); i++)
	{
		m_Threads[i] = std::thread([this]()
			{
				while (!m_Tasks.empty())
				{
					std::function<void()> task;
					if (!TryDequeue(&task)) return;
					task();
				}
			});
	}

	for (size_t i = 0; i < m_Threads.size(); i++)
		m_Threads[i].join();
}

bool TaskBatch::TryDequeue(std::function<void()>* task)
{
	std::unique_lock<std::mutex> lock(m_Mutex); // Lock the mutex and unlock it when the lock goes out of scope

	if (m_Tasks.empty()) return false; // No more tasks to dequeue

	*task = m_Tasks.front();
	m_Tasks.pop();
	return true;
}