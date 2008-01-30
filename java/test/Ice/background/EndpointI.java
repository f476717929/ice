// **********************************************************************
//
// Copyright (c) 2003-2008 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

final class EndpointI extends IceInternal.EndpointI
{
    final static short TYPE_BASE = 100;

    public
    EndpointI(IceInternal.EndpointI endpoint)
    {
        _endpoint = endpoint;
        _configuration = Configuration.getInstance();
    }

    //
    // Marshal the endpoint
    //
    public void
    streamWrite(IceInternal.BasicStream s)
    {
        s.writeShort(type());
        _endpoint.streamWrite(s);
    }

    //
    // Convert the endpoint to its string form
    //
    public String
    _toString()
    {
        return "test-" + _endpoint.toString();
    }

    //
    // Return the endpoint type
    //
    public short
    type()
    {
        return (short)(TYPE_BASE + _endpoint.type());
    }

    //
    // Return the timeout for the endpoint in milliseconds. 0 means
    // non-blocking, -1 means no timeout.
    //
    public int
    timeout()
    {
        return _endpoint.timeout();
    }

    //
    // Return a new endpoint with a different timeout value, provided
    // that timeouts are supported by the endpoint. Otherwise the same
    // endpoint is returned.
    //
    public IceInternal.EndpointI
    timeout(int timeout)
    {
        IceInternal.EndpointI endpoint = _endpoint.timeout(timeout);
        if(endpoint == _endpoint)
        {
            return this;
        }
        else
        {
            return new EndpointI(endpoint);
        }
    }

    //
    // Return a new endpoint with a different connection id.
    //
    public IceInternal.EndpointI
    connectionId(String connectionId)
    {
        IceInternal.EndpointI endpoint = _endpoint.connectionId(connectionId);
        if(endpoint == _endpoint)
        {
            return this;
        }
        else
        {
            return new EndpointI(endpoint);
        }
    }

    //
    // Return true if the endpoints support bzip2 compress, or false
    // otherwise.
    //
    public boolean
    compress()
    {
        return _endpoint.compress();
    }

    //
    // Return a new endpoint with a different compression value,
    // provided that compression is supported by the
    // endpoint. Otherwise the same endpoint is returned.
    //
    public IceInternal.EndpointI
    compress(boolean compress)
    {
        IceInternal.EndpointI endpoint = _endpoint.compress(compress);
        if(endpoint == _endpoint)
        {
            return this;
        }
        else
        {
            return new EndpointI(endpoint);
        }
    }

    //
    // Return true if the endpoint is datagram-based.
    //
    public boolean
    datagram()
    {
        return _endpoint.datagram();
    }

    //
    // Return true if the endpoint is secure.
    //
    public boolean
    secure()
    {
        return _endpoint.secure();
    }

    //
    // Return true if the endpoint type is unknown.
    //
    public boolean
    unknown()
    {
        return _endpoint.unknown();
    }

    //
    // Return a server side transceiver for this endpoint, or null if a
    // transceiver can only be created by an acceptor. In case a
    // transceiver is created, this operation also returns a new
    // "effective" endpoint, which might differ from this endpoint,
    // for example, if a dynamic port number is assigned.
    //
    public IceInternal.Transceiver
    transceiver(IceInternal.EndpointIHolder endpoint)
    {
        IceInternal.Transceiver transceiver = _endpoint.transceiver(endpoint);
        if(endpoint.value == _endpoint)
        {
            endpoint.value = this;
        }
        else
        {
            endpoint.value = new EndpointI(endpoint.value);
        }

        if(transceiver != null)
        {
            return new Transceiver(transceiver);
        }
        else
        {
            return null;
        }
    }

    //
    // Return connectors for this endpoint, or empty list if no connector
    // is available.
    //
    public java.util.List<IceInternal.Connector>
    connectors()
    {
        _configuration.checkConnectorsException();
        java.util.List<IceInternal.Connector> connectors = new java.util.ArrayList<IceInternal.Connector>();
        java.util.Iterator<IceInternal.Connector> p = _endpoint.connectors().iterator();
        while(p.hasNext())
        {
            connectors.add(new Connector(p.next()));
        }
        return connectors;
    }

    public void
    connectors_async(final IceInternal.EndpointI_connectors cb)
    {
        class Callback implements IceInternal.EndpointI_connectors
        {
            public void 
            connectors(java.util.List<IceInternal.Connector> cons)
            {
                java.util.List<IceInternal.Connector> connectors = new java.util.ArrayList<IceInternal.Connector>();
                java.util.Iterator<IceInternal.Connector> p = cons.iterator();
                while(p.hasNext())
                {
                    connectors.add(new Connector(p.next()));
                }
                cb.connectors(connectors);
            }

            public void
            exception(Ice.LocalException exception)
            {
                cb.exception(exception);
            }
        }

        try
        {
            _configuration.checkConnectorsException();
            _endpoint.connectors_async(new Callback());
        }
        catch(Ice.LocalException ex)
        {
            cb.exception(ex);
        }
    }

    public IceInternal.Acceptor
    acceptor(IceInternal.EndpointIHolder endpoint, String adapterName)
    {
        Acceptor p = new Acceptor(_endpoint.acceptor(endpoint, adapterName));
        endpoint.value = new EndpointI(endpoint.value);
        return p;
    }

    public java.util.List<IceInternal.EndpointI>
    expand()
    {
        java.util.List<IceInternal.EndpointI> endps = new java.util.ArrayList<IceInternal.EndpointI>();
        java.util.Iterator<IceInternal.EndpointI> iter = _endpoint.expand().iterator();
        while(iter.hasNext())
        {
            IceInternal.EndpointI endpt = iter.next();
            endps.add(endpt == _endpoint ? this : new EndpointI(endpt));
        }
        return endps;
    }

    public boolean
    equivalent(IceInternal.EndpointI endpoint)
    {
        EndpointI testEndpoint = null;
        try
        {
            testEndpoint = (EndpointI)endpoint;
        }
        catch(ClassCastException ex)
        {
            return false;
        }
        return testEndpoint._endpoint.equivalent(_endpoint);
    }

    public int
    hashCode()
    {
        return _endpoint.hashCode();
    }

    //
    // Compare endpoints for sorting purposes
    //
    public boolean
    equals(java.lang.Object obj)
    {
        try
        {
            return compareTo((IceInternal.EndpointI)obj) == 0;
        }
        catch(ClassCastException ee)
        {
            assert(false);
            return false;
        }
    }

    public int
    compareTo(IceInternal.EndpointI obj) // From java.lang.Comparable
    {
        EndpointI p = null;

        try
        {
            p = (EndpointI)obj;
        }
        catch(ClassCastException ex)
        {
            return type() < obj.type() ? -1 : 1;
        }

        if(this == p)
        {
            return 0;
        }

        return _endpoint.compareTo(p._endpoint);
    }

    private IceInternal.EndpointI _endpoint;
    private Configuration _configuration;
}
