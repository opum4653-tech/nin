#include "crash_handler.hpp"
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <android/log.h>

static void safe_sig_handler(int sig, siginfo_t* info, void* context) {
    const char* sig_name = "UNKNOWN";
    if (sig == SIGSEGV) sig_name = "SIGSEGV (Segmentation Fault - Invalid Memory Access)";
    else if (sig == SIGBUS) sig_name = "SIGBUS (Bus Error - Alignment / Unmapped Region)";
    else if (sig == SIGFPE) sig_name = "SIGFPE (Floating Point Exception)";
    else if (sig == SIGILL) sig_name = "SIGILL (Illegal Instruction)";
    else if (sig == SIGABRT) sig_name = "SIGABRT (Abort Called)";

    void* fault_addr = info ? info->si_addr : nullptr;

    fprintf(stderr, "\n\n=======================================================\n");
    fprintf(stderr, "[-] CRASH INTERCEPTED BY CRASH_GUARD!\n");
    fprintf(stderr, "[-] Signal : %s (%d)\n", sig_name, sig);
    fprintf(stderr, "[-] Address: %p\n", fault_addr);
    fprintf(stderr, "[-] Process cleanly halted.\n");
    fprintf(stderr, "=======================================================\n\n");
    fflush(stderr);

    __android_log_print(ANDROID_LOG_FATAL, "CrashGuard", "[-] Intercepted fatal signal %d (%s) at %p", sig, sig_name, fault_addr);

    _exit(128 + sig);
}

namespace crash_guard {
    void install() {
        struct sigaction sa;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_SIGINFO;
        sa.sa_sigaction = safe_sig_handler;

        sigaction(SIGSEGV, &sa, nullptr);
        sigaction(SIGBUS, &sa, nullptr);
        sigaction(SIGFPE, &sa, nullptr);
        sigaction(SIGILL, &sa, nullptr);
        sigaction(SIGABRT, &sa, nullptr);
    }
}
