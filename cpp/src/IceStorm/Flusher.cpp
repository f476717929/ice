// **********************************************************************
//
// Copyright (c) 2001
// MutableRealms, Inc.
// Huntsville, AL, USA
//
// All Rights Reserved
//
// **********************************************************************

#include <IceUtil/Thread.h>
#include <IceUtil/Monitor.h>

#include <Ice/Ice.h>
#include <Ice/Functional.h>

#include <IceStorm/Flushable.h>
#include <IceStorm/TraceLevels.h>
#include <IceStorm/Flusher.h>

#include <algorithm>
#include <list>

using namespace IceStorm;
using namespace std;

namespace IceStorm
{

typedef std::list<FlushablePtr> FlushableList;

class FlusherThread : public IceUtil::Thread, public IceUtil::Monitor<IceUtil::Mutex>
{
public:

    FlusherThread(const Ice::CommunicatorPtr& communicator, const TraceLevelsPtr& traceLevels) :
	_traceLevels(traceLevels),
        _destroy(false)
    {
	Ice::PropertiesPtr properties = communicator->getProperties();
	string value;
	value = properties->getProperty("IceStorm.Flush.Timeout");
	if (!value.empty())
	{
	    _flushTime = atoi(value.c_str());
	    if (_flushTime < 100)
	    {
		_flushTime = 100; // Minimum of 100 ms
	    }
	}
	else
	{
	    _flushTime = 1000; // Default of 1 second
	}
    }
    
    ~FlusherThread()
    {
    }

    virtual void
    run()
    {
	IceUtil::Monitor<IceUtil::Mutex>::Lock sync(*this);
	while (!_destroy)
	{
	    long tout = calcTimeout();
	    if (tout == 0)
	    {
		wait();
	    }
	    else
	    {
		timedwait(tout);
	    }
	    if (_destroy)
	    {
		continue;
	    }
	    flushAll();
	}
    }

    void
    destroy()
    {
	IceUtil::Monitor<IceUtil::Mutex>::Lock sync(*this);
	_destroy = true;
	notify();
    }

    //
    // It would be possible to write add/remove in such a way as to
    // avoid blocking while flushing by having a queue of actions
    // which are only performed before flushing. For now, however,
    // this issue is ignored.
    //
    void
    add(const FlushablePtr& subscriber)
    {
	IceUtil::Monitor<IceUtil::Mutex>::Lock sync(*this);
	bool isEmpty = _subscribers.empty();
	_subscribers.push_back(subscriber);

	//
	// If the set of subscribers was previously empty then wake up
	// the flushing thread since it will be waiting indefinitely
	//
	if (isEmpty)
	{
	    notify();
	}
    }

    void
    remove(const FlushablePtr& subscriber)
    {
	IceUtil::Monitor<IceUtil::Mutex>::Lock sync(*this);
	_subscribers.remove(subscriber);
    }


private:

    void
    flushAll()
    {
	// This is always called with the monitor locked
	//IceUtil::Monitor<IceUtil::Mutex>::Lock sync(*this);

	//
	// Using standard algorithms I don't think there is a way to
	// do this in one pass. For instance, I thought about using
	// remove_if - but the predicate needs to be a pure function
	// (see Meyers for details). If this is fixed then fix TopicI
	// also.
	//
        // remove_if doesn't work with handle types. remove_if also
        // isn't present in the STLport implementation
        //
        // _subscribers.remove_if(IceUtil::constMemFun(&Flushable::inactive));
        //
        _subscribers.erase(remove_if(_subscribers.begin(), _subscribers.end(),
				     IceUtil::constMemFun(&Flushable::inactive)), _subscribers.end());
	for_each(_subscribers.begin(), _subscribers.end(), IceUtil::voidMemFun(&Flushable::flush));

	//
	// Trace after the flush so that the correct number of objects
	// are displayed
	//
	if (_traceLevels->flush > 0)
	{
	    ostringstream s;
	    s << _subscribers.size() << " object(s)";
	    
	    _traceLevels->logger->trace(_traceLevels->flushCat, s.str());
	}
    }

    long
    calcTimeout()
    {
	return (_subscribers.empty()) ? 0 : _flushTime;
    }

    TraceLevelsPtr _traceLevels;

    FlushableList _subscribers;
    bool _destroy;
    long _flushTime;
};

} // End namespace IceStorm

Flusher::Flusher(const Ice::CommunicatorPtr& communicator, const TraceLevelsPtr& traceLevels)
{
    _thread = new FlusherThread(communicator, traceLevels);
    _thread->start();
}

Flusher::~Flusher()
{
    _thread->destroy();
    _thread->getThreadControl().join();
}

void
Flusher::add(const FlushablePtr& subscriber)
{
    _thread->add(subscriber);
}

void
Flusher::remove(const FlushablePtr& subscriber)
{
    _thread->remove(subscriber);
}

void
Flusher::stopFlushing()
{
    _thread->destroy();
}

