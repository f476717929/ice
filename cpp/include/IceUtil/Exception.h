// **********************************************************************
//
// Copyright (c) 2001
// MutableRealms, Inc.
// Huntsville, AL, USA
//
// All Rights Reserved
//
// **********************************************************************

#ifndef ICE_UTIL_EXCEPTION_H
#define ICE_UTIL_EXCEPTION_H

#include <IceUtil/Config.h>

namespace IceUtil
{

class ICE_UTIL_API Exception
{
public:

    Exception();
    Exception(const char*, int);
    virtual ~Exception();
    virtual std::string ice_name() const;
    virtual void ice_print(std::ostream&) const;
    virtual Exception* ice_clone() const;
    virtual void ice_throw() const;
    const char* ice_file() const;
    int ice_line() const;
    
private:
    
    const char* _file;
    int _line;
};

ICE_UTIL_API std::ostream& operator<<(std::ostream&, const Exception&);

class ICE_UTIL_API NullHandleException : public Exception
{
public:
    
    NullHandleException(const char*, int);
    virtual std::string ice_name() const;
    virtual Exception* ice_clone() const;
    virtual void ice_throw() const;
};

class ICE_UTIL_API SyscallException : public Exception
{
public:

    SyscallException(const std::string&, const char*, int);
    virtual std::string ice_name() const;
    virtual void ice_print(std::ostream&) const;
    virtual Exception* ice_clone() const;
    virtual void ice_throw() const;

#ifdef WIN32
    //
    // IceInternal::errorToString does the same thing... refactor?
    //
    static std::string errorToString(DWORD);
#endif

private:

    std::string _error;
};

class ICE_UTIL_API LockedException : public Exception
{
public:

    LockedException(const char*, int);
    virtual std::string ice_name() const;
    virtual Exception* ice_clone() const;
    virtual void ice_throw() const;
};

}

#endif
