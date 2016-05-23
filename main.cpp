#include "Scheduler/scheduler.h"

int main(int argc, char **argv) {
    Scheduler scheduler(argc, argv);
    return scheduler.run();
}
