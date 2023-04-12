#include "task.h"
#include <iostream>       // std::cout
#include <thread>         // std::thread
#include <chrono>		  //ms
#include <cassert>

TaskManager TaskManager::foreground;
TaskManager TaskManager::background;

TaskManager::TaskManager()
{
	must_loop = false;
	_thread = NULL;
}

void TaskManager::loop()
{
	using namespace std::chrono_literals;
	std::cout << "Starting Task Manager ..." << std::endl;

	while (must_loop)
	{
		if (pending_tasks.empty())
		{
			std::this_thread::sleep_for(10ms);
			continue;
		}

		fetchTask();
	}

	std::cout << "Ending Task Manager" << std::endl;
}

void TaskManager::fetchTask()
{
	Task* task = NULL;
	try
	{
		//lock
		const std::lock_guard<std::mutex> lock(tasks_mutex);
		if (pending_tasks.empty())
			return;
		task = pending_tasks.front();
		pending_tasks.pop_front();
		//unlock after finishing scope
	}
	catch (std::logic_error&) {
		std::cout << "[exception caught]\n";
	}

	if (task)
	{
		task->onExecute();
		delete task;
		task = NULL;
	}
}

void thread_loop_func(TaskManager* manager)
{
	manager->loop();
	//join?
}

void TaskManager::startThread()
{
	assert(!_thread && "TaskManager already in a thread");
	must_loop = true;
	_thread = new std::thread(thread_loop_func, this);
}

void TaskManager::addTask(Task* task)
{
	//block pending_tasks
	const std::lock_guard<std::mutex> lock(tasks_mutex);
	pending_tasks.push_back(task);
	//release pending_tasks automatically
}