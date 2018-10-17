
#include "stdafx.h"
#include "ThreadPool.h"


cWorkerThread::cWorkerThread()
	: cIsThread("cWorkerThread")
	, m_task_cnt(0)
{

}

cWorkerThread::~cWorkerThread()
{
	m_ShouldTerminate = true;
	m_QueueNonempty.Set();
	Wait();
}

void cWorkerThread::Execute(void)
{
	for (;;)
	{
		cCSLock Lock(m_CS);
		while (!m_ShouldTerminate && (m_Queue.size() == 0))
		{
			cCSUnlock Unlock(Lock);
			m_QueueNonempty.Wait();
		}
		if (m_ShouldTerminate)
		{
			return;
		}
		ASSERT(!m_Queue.empty());

		cTask * tast = m_Queue.front();
		m_Queue.pop_front();
		Lock.Unlock();

		tast->run();

		delete tast;
		tast = nullptr;

		m_task_cnt++;

	}  // for (-ever)
}

void cWorkerThread::PushTask(cTask* pTask)
{
	cCSLock LOCK(m_CS);
	m_Queue.push_back(pTask);
	m_QueueNonempty.Set();
}


cThreadPool::cThreadPool()
	: m_worker_size(0)
	, m_worker_list(nullptr)	
{
}


cThreadPool::~cThreadPool()
{
	Destory();
}

int cThreadPool::Init(UInt32 worker_size/* = 4*/)
{
	m_worker_size = worker_size;
	m_worker_list = new cWorkerThread[m_worker_size];
	if (!m_worker_list) 
	{
		return 1;
	}

	for (UInt32 i = 0; i < m_worker_size; i++)
	{
		m_worker_list[i].SetThreadIdx(i);
		m_worker_list[i].Start();
	}

	return 0;
}

void cThreadPool::AddTask(cTask* pTask)
{
	UInt32 thread_idx = rand() % m_worker_size;
	m_worker_list[thread_idx].PushTask(pTask);
}

void cThreadPool::Destory()
{
	if (m_worker_list)
		delete[] m_worker_list;

	m_worker_list = nullptr;
}
