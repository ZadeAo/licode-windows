/*
* ThreadPool.h
*
*  Created on: 2015Äê`ÔÂ25ÈÕ
*  Author: junjun
*/

#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#pragma once

#include "OSSupport/IsThread.h"

class COMMON_API cTask 
{
public:
	cTask(){}
	virtual ~cTask(){}

	virtual void run() = 0;
private:
};



class COMMON_API cWorkerThread :public cIsThread
{
public:
	cWorkerThread();
	virtual ~cWorkerThread();

	virtual void Execute(void);
	void PushTask(cTask* pTask);
	void SetThreadIdx(UInt32 idx) { m_thread_idx = idx; }
private:
	UInt32		m_task_cnt;
	UInt32		m_thread_idx;

	cCriticalSection  m_CS;
	cEvent            m_QueueNonempty;
	std::list<cTask*> m_Queue;
};



class COMMON_API cThreadPool
{
public:
	cThreadPool();
	virtual ~cThreadPool();

	int Init(UInt32 worker_size = 4);
	void AddTask(cTask* pTask);
	void Destory();
private:
	UInt32 		m_worker_size;
	cWorkerThread* 	m_worker_list;
};


#endif /* THREADPOOL_H_ */