#include "Timer.h"

#include <signal.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <stdexcept>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

using namespace std;


Timer::Timer(int sec, int usec, Action (*handler)(timer_t id)) {
	timespec ts, tm;
	sigset_t smask;
	siginfo_t sinfo;
	sigevent sigev;
	struct sigaction sa;
	itimerspec iv;

	_usrhandler = handler;
	_sec = sec;
	_usec = usec;

	sigemptyset(&smask);
	sigprocmask(SIG_SETMASK, &smask, NULL);
	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = &Timer::_handler;
	if (sigaction(SIGRTMIN, &sa, NULL) == -1) {
		throw runtime_error("fail sigaction");
	}

	sigev.sigev_notify = SIGEV_SIGNAL;
	sigev.sigev_signo = SIGRTMIN;
	sigev.sigev_value.sival_int = 1;
	sigev.sigev_value.sival_ptr = (void*)handler;
	if (timer_create(CLOCK_REALTIME, &sigev, &_id) == -1) {
		throw runtime_error("fail to create timer");
	}

	iv.it_value.tv_sec = sec;
	iv.it_value.tv_nsec = usec * 1000;
	iv.it_interval.tv_sec = sec;
	iv.it_interval.tv_nsec = usec * 1000;
	if (timer_settime(_id, 0, &iv, NULL) == -1) {
		throw runtime_error("fail to start timer");
	}
}

Timer::~Timer() {
	timer_delete(_id);
}

void Timer::stop() {
	itimerspec iv = {0};
	timer_settime(_id, 0, &iv, NULL);
}

void Timer::start() {
	itimerspec iv;
	iv.it_value.tv_sec = _sec;
	iv.it_value.tv_nsec = _usec * 1000;
	iv.it_interval.tv_sec = _sec;
	iv.it_interval.tv_nsec = _usec * 1000;
	timer_settime(_id, 0, &iv, NULL);
}

void Timer::start(int sec, int usec) {
	itimerspec iv;
	iv.it_value.tv_sec = sec;
	iv.it_value.tv_nsec = usec * 1000;
	iv.it_interval.tv_sec = sec;
	iv.it_interval.tv_nsec = usec * 1000;
	timer_settime(_id, 0, &iv, NULL);
}

timer_t Timer::getid() {
	return _id;
}

void Timer::_handler(int sig, siginfo_t* info, void* context) {
	itimerspec iv = {0};
	int ncall = timer_getoverrun((timer_t)(&info->si_timerid));
	if (ncall == -1 || ncall == 0) {
		ncall = 1;
	}

	Timer::Action action;
	for (int i = 0; i < ncall; i++) {
		action = ((Timer::Action (*)(timer_t))info->si_value.sival_ptr) ((timer_t)(&info->si_timerid));
		if (action == Timer::Action::STOP) {
			timer_settime((timer_t)(&info->si_timerid), 0, &iv, NULL);
			break;
		}
	}

}
