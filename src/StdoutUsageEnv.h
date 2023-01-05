#pragma once
#include "BasicUsageEnvironment.hh"

class StdoutUsageEnv :
    public BasicUsageEnvironment
{
public:
    static StdoutUsageEnv* createNew(TaskScheduler& taskScheduler);

    // redefined virtual functions:
    virtual UsageEnvironment& operator<<(char const* str);
    virtual UsageEnvironment& operator<<(int i);
    virtual UsageEnvironment& operator<<(unsigned u);
    virtual UsageEnvironment& operator<<(double d);
    virtual UsageEnvironment& operator<<(void* p);

protected:
    StdoutUsageEnv(TaskScheduler& taskScheduler);
    // called only by "createNew()" (or subclass constructors)
    virtual ~StdoutUsageEnv();
};

