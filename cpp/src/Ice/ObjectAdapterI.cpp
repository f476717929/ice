// **********************************************************************
//
// Copyright (c) 2001
// MutableRealms, Inc.
// Huntsville, AL, USA
//
// All Rights Reserved
//
// **********************************************************************

#include <Ice/ObjectAdapterI.h>
#include <Ice/ServantLocator.h>
#include <Ice/Instance.h>
#include <Ice/Proxy.h>
#include <Ice/ProxyFactory.h>
#include <Ice/Reference.h>
#include <Ice/Endpoint.h>
#include <Ice/ConnectionFactory.h>
#include <Ice/Exception.h>
#include <Ice/Properties.h>
#include <Ice/Functional.h>

#ifdef WIN32
#   include <sys/timeb.h>
#else
#   include <sys/time.h>
#endif

using namespace std;
using namespace Ice;
using namespace IceInternal;

string
Ice::ObjectAdapterI::getName()
{
    return _name; // _name is immutable
}

CommunicatorPtr
Ice::ObjectAdapterI::getCommunicator()
{
    return _instance->communicator(); // _instance is immutable
}

void
Ice::ObjectAdapterI::activate()
{
    IceUtil::Mutex::Lock sync(*this);

    if (_collectorFactories.empty())
    {
	throw ObjectAdapterDeactivatedException(__FILE__, __LINE__);
    }

    for_each(_collectorFactories.begin(), _collectorFactories.end(), Ice::voidMemFun(&IncomingConnectionFactory::activate));
}

void
Ice::ObjectAdapterI::hold()
{
    IceUtil::Mutex::Lock sync(*this);

    if (_collectorFactories.empty())
    {
	throw ObjectAdapterDeactivatedException(__FILE__, __LINE__);
    }

    for_each(_collectorFactories.begin(), _collectorFactories.end(), Ice::voidMemFun(&IncomingConnectionFactory::hold));
}

void
Ice::ObjectAdapterI::deactivate()
{
    IceUtil::Mutex::Lock sync(*this);

    if (_collectorFactories.empty())
    {
	//
        // Ignore deactivation requests if the Object Adapter has
        // already been deactivated.
	//
	return;
    }

    for_each(_collectorFactories.begin(), _collectorFactories.end(), Ice::voidMemFun(&IncomingConnectionFactory::destroy));
    _collectorFactories.clear();

    _activeServantMap.clear();
    _activeServantMapHint = _activeServantMap.end();

    for_each(_locatorMap.begin(), _locatorMap.end(),
	     Ice::secondVoidMemFun<string, ServantLocator>(&ServantLocator::deactivate));
    _locatorMap.clear();
    _locatorMapHint = _locatorMap.end();
}

ObjectPrx
Ice::ObjectAdapterI::add(const ObjectPtr& object, const Identity& ident)
{
    IceUtil::Mutex::Lock sync(*this);

    if (_collectorFactories.empty())
    {
	throw ObjectAdapterDeactivatedException(__FILE__, __LINE__);
    }

    _activeServantMapHint = _activeServantMap.insert(_activeServantMapHint, make_pair(ident, object));

    return newProxy(ident);
}

ObjectPrx
Ice::ObjectAdapterI::addTemporary(const ObjectPtr& object)
{
    IceUtil::Mutex::Lock sync(*this);

    if (_collectorFactories.empty())
    {
	throw ObjectAdapterDeactivatedException(__FILE__, __LINE__);
    }

    ostringstream s;

#ifdef WIN32
    struct _timeb tb;
    _ftime(&tb);
    s << hex << '.' << tb.time << '.' << tb.millitm << '.' << rand();
#else
    timeval tv;
    gettimeofday(&tv, 0);
    s << hex << '.' << tv.tv_sec << '.' << tv.tv_usec / 1000 << '.' << rand();
#endif

    Identity ident;
    ident.name = s.str();

    _activeServantMapHint = _activeServantMap.insert(_activeServantMapHint, make_pair(ident, object));

    return newProxy(ident);
}

void
Ice::ObjectAdapterI::remove(const Identity& ident)
{
    IceUtil::Mutex::Lock sync(*this);

    if (_collectorFactories.empty())
    {
	throw ObjectAdapterDeactivatedException(__FILE__, __LINE__);
    }

    _activeServantMap.erase(ident);
    _activeServantMapHint = _activeServantMap.end();
}

void
Ice::ObjectAdapterI::addServantLocator(const ServantLocatorPtr& locator, const string& prefix)
{
    IceUtil::Mutex::Lock sync(*this);

    if (_collectorFactories.empty())
    {
	throw ObjectAdapterDeactivatedException(__FILE__, __LINE__);
    }

    _locatorMapHint = _locatorMap.insert(_locatorMapHint, make_pair(prefix, locator));
}

void
Ice::ObjectAdapterI::removeServantLocator(const string& prefix)
{
    IceUtil::Mutex::Lock sync(*this);

    if (_collectorFactories.empty())
    {
	throw ObjectAdapterDeactivatedException(__FILE__, __LINE__);
    }

    map<string, ::Ice::ServantLocatorPtr>::iterator p = _locatorMap.end();
    
    if (_locatorMapHint != _locatorMap.end())
    {
	if (_locatorMapHint->first == prefix)
	{
	    p = _locatorMapHint;
	}
    }
    
    if (p == _locatorMap.end())
    {
	p = _locatorMap.find(prefix);
    }
    
    if (p != _locatorMap.end())
    {
	p->second->deactivate();
	_locatorMap.erase(p);
	_locatorMapHint = _locatorMap.end();
    }
}

ServantLocatorPtr
Ice::ObjectAdapterI::findServantLocator(const string& prefix)
{
    IceUtil::Mutex::Lock sync(*this);
    
    if (_collectorFactories.empty())
    {
	throw ObjectAdapterDeactivatedException(__FILE__, __LINE__);
    }

    map<string, ::Ice::ServantLocatorPtr>::iterator p = _locatorMap.end();
    
    if (_locatorMapHint != _locatorMap.end())
    {
	if (_locatorMapHint->first == prefix)
	{
	    p = _locatorMapHint;
	}
    }
    
    if (p == _locatorMap.end())
    {
	p = _locatorMap.find(prefix);
    }
    
    if (p != _locatorMap.end())
    {
	_locatorMapHint = p;
	return p->second;
    }
    else
    {
	return 0;
    }
}

ObjectPtr
Ice::ObjectAdapterI::identityToServant(const Identity& ident)
{
    IceUtil::Mutex::Lock sync(*this);

    if (_activeServantMapHint != _activeServantMap.end())
    {
	if (_activeServantMapHint->first == ident)
	{
	    return _activeServantMapHint->second;
	}
    }
    
    ObjectDict::iterator p = _activeServantMap.find(ident);
    if (p != _activeServantMap.end())
    {
	_activeServantMapHint = p;
	return p->second;
    }
    else
    {
	return 0;
    }
}

ObjectPtr
Ice::ObjectAdapterI::proxyToServant(const ObjectPrx& proxy)
{
    ReferencePtr ref = proxy->__reference();
    return identityToServant(ref->identity);
}

ObjectPrx
Ice::ObjectAdapterI::createProxy(const Identity& ident)
{
    IceUtil::Mutex::Lock sync(*this);
    
    if (_collectorFactories.empty())
    {
	throw ObjectAdapterDeactivatedException(__FILE__, __LINE__);
    }
    
    return newProxy(ident);
}

Ice::ObjectAdapterI::ObjectAdapterI(const InstancePtr& instance, const string& name, const string& endpts) :
    _instance(instance),
    _name(name),
    _activeServantMapHint(_activeServantMap.end()),
    _locatorMapHint(_locatorMap.end())
{
    string s(endpts);
    transform(s.begin(), s.end(), s.begin(), tolower);

    string::size_type beg = 0;
    string::size_type end;
    
    __setNoDelete(true);
    try
    {
	while (true)
	{
	    end = s.find(':', beg);
	    if (end == string::npos)
	    {
		end = s.length();
	    }
	    
	    if (end == beg)
	    {
		throw EndpointParseException(__FILE__, __LINE__);
	    }
	    
	    string es = s.substr(beg, end - beg);
	   
	    //
	    // Don't store the endpoint in the adapter. The Collector
	    // might change it, for example, to fill in the real port
	    // number if a zero port number is given.
	    //
	    EndpointPtr endp = Endpoint::endpointFromString(instance, es);
	    _collectorFactories.push_back(new IncomingConnectionFactory(instance, endp, this));
	    
	    if (end == s.length())
	    {
		break;
	    }
	    
	    beg = end + 1;
	}
    }
    catch(...)
    {
	if (!_collectorFactories.empty())
	{
	    deactivate();
	}
	__setNoDelete(false);
	throw;
    }
    __setNoDelete(false);
    
    if (_collectorFactories.empty())
    {
	throw EndpointParseException(__FILE__, __LINE__);
    }

    string value = _instance->properties()->getProperty("Ice.PrintAdapterReady");
    if (atoi(value.c_str()) >= 1)
    {
	cout << _name << " ready" << endl;
    }
}

Ice::ObjectAdapterI::~ObjectAdapterI()
{
    if (!_collectorFactories.empty())
    {
	deactivate();
    }
}

ObjectPrx
Ice::ObjectAdapterI::newProxy(const Identity& ident)
{
    vector<EndpointPtr> endpoints;
    transform(_collectorFactories.begin(), _collectorFactories.end(), back_inserter(endpoints),
	      Ice::constMemFun(&IncomingConnectionFactory::endpoint));
    
    ReferencePtr reference = new Reference(_instance, ident, "", Reference::ModeTwoway, false, endpoints, endpoints);
    return _instance->proxyFactory()->referenceToProxy(reference);
}

bool
Ice::ObjectAdapterI::isLocal(const ObjectPrx& proxy)
{
    ReferencePtr ref = proxy->__reference();
    vector<EndpointPtr>::const_iterator p;
    for (p = ref->endpoints.begin(); p != ref->endpoints.end(); ++p)
    {
	vector<IncomingConnectionFactoryPtr>::const_iterator q;
	for (q = _collectorFactories.begin(); q != _collectorFactories.end(); ++q)
	{
	    if ((*q)->equivalent(*p))
	    {
		return true;
	    }
	}
    }

    return false;
}
