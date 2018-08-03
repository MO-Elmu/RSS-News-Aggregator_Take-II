/**
 * File: thread-pool.cc
 * --------------------
 * Presents the implementation of the ThreadPool class.
 */

#include "thread-pool.h"
#include <iostream>
using namespace std;

void ThreadPool::dispatcher(){
	while(true){
		//lock_guard<mutex> lgq(qLock);
		qLock.lock();
		qCond.wait(qLock,[this]{if(thunkQ.empty()) qCond.notify_all(); return !thunkQ.empty(); });
		qLock.unlock();
		if(finished) break;
                threads.wait();
                for(size_t i=0; i < wts.size(); i++){
			lock_guard<mutex> lqwts(wtsLock);
			if(wts[i].second == true) {
				wts[i].second = false;
                                lock_guard<mutex> lgq(qLock);
				lock_guard<mutex> lgfuncs(funcsLock);
				functions[i].first = thunkQ.front();
				thunkQ.pop();
                                functions[i].second->signal();
				break;
                         }
		}	
	}
}
void ThreadPool::worker(size_t workerID /*const function<void(void)>& thunk*/){
	while(true){
		functions[workerID].second->wait();
                if(finished) break;
		funcsLock.lock();
		function<void(void)> thunk = functions[workerID].first;
		funcsLock.unlock();
		thunk();
		lock_guard<mutex> lqwts(wtsLock);
		wts[workerID].second = true;
		threads.signal();
		workerCond.notify_all();
        }
}

ThreadPool::ThreadPool(size_t numThreads) : wts(numThreads), functions(numThreads), threads(numThreads),finished(false){
	dt = thread([this]() {
	  dispatcher();
	});
	for (size_t workerID = 0; workerID < numThreads; workerID++) {
		if(functions[workerID].second == nullptr) functions[workerID].second.reset(new semaphore());
		wts[workerID].first = thread([this](size_t workerID) {
			worker(workerID);
		}, workerID);
		wts[workerID].second = true;
	}
}

void ThreadPool::schedule(const function<void(void)>& thunk) {
	qLock.lock();
	thunkQ.push(thunk);
	qLock.unlock();
	qCond.notify_all();	
}

void ThreadPool::wait() {
	lock_guard<mutex> lgq(qLock);
	qCond.wait(qLock,[this]{return thunkQ.empty(); });
	for(size_t i=0; i<wts.size(); i++){
		lock_guard<mutex> lgwts(wtsLock);
		workerCond.wait(wtsLock,[this,i]{return wts[i].second == true;});
	}
}


ThreadPool::~ThreadPool() {
	this->wait();
        auto thunk = [] () {return;}; //dummy function to fool the dispatcher int exiting
        finished = true;
        qLock.lock();
        thunkQ.push(thunk);
        qLock.unlock(); 
        qCond.notify_all();
	dt.join();
	for(size_t i=0; i<wts.size(); i++){
                functions[i].second->signal();
		wts[i].first.join();
        }
}

