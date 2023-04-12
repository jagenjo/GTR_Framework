#pragma once

#include <vector>
#include <list>
#include <mutex>
#include <thread>         // std::thread
#include <functional>

//any task executed in BG should inherit from this one
class Task {
public:
	std::function<void()> callback;
	Task() { callback = NULL; };
	Task(std::function<void()> func) { callback = func; };
	virtual ~Task() {};
	virtual void onExecute() { if (callback) callback(); }
};

class TaskManager {
public:
	std::list<Task*> pending_tasks;
	std::mutex tasks_mutex;  // protects pending_tasks
	bool must_loop;
	std::thread* _thread;

	static TaskManager foreground;
	static TaskManager background;

	TaskManager();
	void addTask(Task* task);
	void fetchTask();
	void loop();
	void startThread();
};