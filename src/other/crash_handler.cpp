#include "crash_handler.hpp"
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <android/log.h>

static void safe_sig_handler(int sig, siginfo_t* info, void* context) {
    const char* sig_name = "UNKNOWN";
    if (sig == SIGSEGV) sig_name = "SIGSEGV (Invalid Memory Access / Null Pointer)";
    else if (sig == SIGBUS) sig_name = "SIGBUS (Alignment / Unmapped Memory Fault)";
    else if (sig == SIGFPE) sig_name = "SIGFPE (Arithmetic Exception)";
    else if (sig == SIGILL) sig_name = "SIGILL (Illegal Instruction)";
    else if (sig == SIGABRT) sig_name = "SIGABRT (Abort Signal)";

    void* fault_addr = info ? info->si_addr : nullptr;

    fprintf(stderr, "\n=======================================================\n");
    fprintf(stderr, "[-] CRASH INTERCEPTED BY CRASH_GUARD!\n");
    fprintf(stderr, "[-] Signal : %s (%d)\n", sig_name, sig);
    fprintf(stderr, "[-] Address: %p\n", fault_addr);
    fprintf(stderr, "[-] Intercepted safely. Halting process cleanly.\n");
    fprintf(stderr, "=======================================================\n\n");
    fflush(stderr);

    __android_log_print(ANDROID_LOG_FATAL, "CrashGuard", "[-] Intercepted fatal signal %d (%s) at %p", sig, sig_name, fault_addr);

    // Clean exit to prevent terminal error 139 / Segmentation fault notice
    _exit(0);
}

namespace crash_guard {
    void install() {
        // Allocate dedicated alternate signal stack so stack overflows can be caught safely
        static uint8_t s_alt_stack[SIGSTKSZ * 4];
        stack_t ss{};
        memset(&ss, 0, sizeof(ss));
        ss.ss_sp = s_alt_stack;
        ss.ss_size = sizeof(s_alt_stack);
        ss.ss_flags = 0;
        sigaltstack(&ss, nullptr);

        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
        sa.sa_sigaction = safe_sig_handler;

        sigaction(SIGSEGV, &sa, nullptr);
        sigaction(SIGBUS, &sa, nullptr);
        sigaction(SIGFPE, &sa, nullptr);
        sigaction(SIGILL, &sa, nullptr);
        sigaction(SIGABRT, &sa, nullptr);
    }
}
