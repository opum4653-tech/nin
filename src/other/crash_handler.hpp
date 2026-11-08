#pragma once

namespace crash_guard {
    // Installs a global signal handler to catch segmentation faults (SIGSEGV),
    // bus errors (SIGBUS), and illegal instructions (SIGILL) with descriptive logs.
    void install();
}
