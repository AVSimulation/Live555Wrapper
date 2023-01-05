#include "StdoutUsageEnv.h"

#include <stdio.h>

StdoutUsageEnv::StdoutUsageEnv(TaskScheduler& taskScheduler) : BasicUsageEnvironment(taskScheduler) 
{
}

StdoutUsageEnv::~StdoutUsageEnv() {
}

StdoutUsageEnv* StdoutUsageEnv::createNew(TaskScheduler& taskScheduler) {
    return new StdoutUsageEnv(taskScheduler);
}

UsageEnvironment& StdoutUsageEnv::operator<<(char const* str) {
    if (str == NULL) str = "(NULL)"; // sanity check
    fprintf(stdout, "%s", str);
    return *this;
}

UsageEnvironment& StdoutUsageEnv::operator<<(int i) {
    fprintf(stdout, "%d", i);
    return *this;
}

UsageEnvironment& StdoutUsageEnv::operator<<(unsigned u) {
    fprintf(stdout, "%u", u);
    return *this;
}

UsageEnvironment& StdoutUsageEnv::operator<<(double d) {
    fprintf(stdout, "%f", d);
    return *this;
}

UsageEnvironment& StdoutUsageEnv::operator<<(void* p) {
    fprintf(stdout, "%p", p);
    return *this;
}
