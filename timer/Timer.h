/* Author: Jodzik */
#ifndef TIMER_H_
#define TIMER_H_

#include <signal.h>
#include <time.h>

class Timer {
public:
	enum class Action {
		CONTINUE,
		STOP
	};

	Timer(int sec, int usec, Action (*handler)(timer_t id));
	virtual ~Timer();

	void stop();
	virtual void start();
	virtual void start(int sec, int usec);
	timer_t getid();

private:
	timer_t _id;
	int _sec;
	int _usec;

	Action (*_usrhandler)(timer_t id);

	static void _handler(int sig, siginfo_t* info, void* context);
};


#endif