#include <thread>
#include <chrono>
#include "board.h"
#include "component.h"
#include "spinlock_barrier.h"
#include "events.h"
#include "output.h"
#include <iostream>


bool* Board::buffer1(nullptr);
bool* Board::buffer2(nullptr);
bool* Board::buffer3(nullptr);

bool* Board::readBuffer(nullptr);
bool* Board::writeBuffer(nullptr);
bool* Board::wipeBuffer(nullptr);

Component** Board::components(nullptr);
Link** Board::links(nullptr);
int Board::threadCount(1);
bool Board::manualClock(false);
unsigned long long Board::lastCaptureTick(0);
unsigned long long Board::tick(0);
unsigned long long Board::currentSpeed(0);
size_t Board::componentCount(0);
size_t Board::linkCount(0);
std::thread** Board::threads = nullptr;
SpinlockBarrier* Board::barrier(nullptr);
Events::Event<> Board::tickEvent;
std::chrono::high_resolution_clock::time_point Board::lastCapture(std::chrono::high_resolution_clock::now());

Board::State Board::currentState(Board::Uninitialized);

void Board::init(Component** components, Link** links, int componentCount, int linkCount)
{
	Board::init(components, links, componentCount, linkCount, 1, false);
}

void Board::init(Component** components, Link** links, int componentCount, int linkCount, bool manualClock)
{
	Board::init(components, links, componentCount, linkCount, 1, manualClock);
}

void Board::init(Component** components, Link** links, int componentCount, int linkCount, int threadCount)
{
	Board::init(components, links, componentCount, linkCount, threadCount, false);
}

void Board::init(Component** components, Link** links, int componentCount, int linkCount, int threadCount, bool manualClock)
{
	if (Board::currentState != Board::Uninitialized)
		return;

	Board::components = components;
	Board::links = links;
	Board::threadCount = threadCount;
	Board::manualClock = manualClock;
	Board::componentCount = componentCount;
	Board::linkCount = linkCount;

	Board::buffer1 = new bool[componentCount] { 0 };
	Board::buffer2 = new bool[componentCount] { 0 };
	Board::buffer3 = new bool[componentCount] { 0 };

	Board::readBuffer = Board::buffer1;
	Board::writeBuffer = Board::buffer2;
	Board::wipeBuffer = Board::buffer3;
	
	Board::currentState = Board::Stopped;

	Board::threads = new std::thread*[threadCount] { nullptr };
	Board::lastCapture = std::chrono::high_resolution_clock::now();

	unsigned long long& counter = Board::tick;
	std::chrono::high_resolution_clock::time_point& lastCapture = Board::lastCapture;
	Board::barrier = new SpinlockBarrier(Board::threadCount, [&counter, &lastCapture]() {
		if (Board::currentState == Board::Stopping) {
			Board::currentState = Board::Stopped;
			return;
		}
		
		tickEvent.emit(nullptr, Events::EventArgs());

		bool* readPointer(Board::readBuffer);

		Board::readBuffer = Board::writeBuffer;
		Board::writeBuffer = Board::wipeBuffer;
		Board::wipeBuffer = readPointer;

		if (Board::getManualClock()) {
			//TODO: wait for confirmation
		}

		counter++;
		long long diff = (std::chrono::high_resolution_clock::now() - lastCapture).count();
		if (diff > 10e8) {
			Board::currentSpeed = ((counter - Board::lastCaptureTick) * (unsigned long)10e8) / diff;
			lastCapture = std::chrono::high_resolution_clock::now();
			Board::lastCaptureTick = counter;
		}
	});
}

int Board::getThreadCount() {
	return threadCount;
}

Component** Board::getComponents() {
	return components;
}

Link** Board::getLinks()
{
	return links;
}

bool Board::getManualClock() {
	return manualClock;
}

Board::State Board::getCurrentState() {
	return currentState;
}

unsigned long long int Board::getCurrentTick()
{
	return tick;
}

void Board::stop() {
	if (Board::currentState != Board::Running)
		return;

	Board::currentState = Board::Stopping;
	for (int i = 0; i < Board::threadCount; i++) {
		Board::threads[i]->join();
	}
}

void Board::start() {
	if (Board::currentState != Board::Stopped)
		return;

	for (int i = 0; i < threadCount; i++) {
		if(Board::threads[i] != nullptr)
			delete Board::threads[i];

		Board::currentState = Board::Running;
		Board::threads[i] = new std::thread([](int id) {
			while (true) {
				if (Board::currentState == Board::Stopped)
					return;

				for (int i = id; i < Board::componentCount; i += Board::threadCount) {
					if (Board::readBuffer[i])
						Board::components[i]->compute();
					Board::wipeBuffer[i] = false;
				}
				Board::barrier->wait();
			}
		}, i);
	}
}
