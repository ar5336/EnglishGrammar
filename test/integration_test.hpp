#ifndef INTEGRATION_TEST
#define INTEGRATION_TEST

#include "test.hpp"

class IntegrationTester
{
private:
    ParserTester* tester;
public:
    void run_tests();
};

#endif