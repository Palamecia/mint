#include "scheduler/scheduler.h"

using namespace mint;

int main(int argc, char **argv) {
    Scheduler scheduler(argc, argv);
    return scheduler.run();
}
