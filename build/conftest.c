#if ARCHTEST
#if __i386__
#undef i386
i386
#elif __x86_64__
#undef x86_64
x86_64
#else
#undef non_x86
non_x86
#endif
#else
#ifdef HAS_TLS
__thread
#endif
int a;
int main(void){volatile int b = a; return b;}
#endif
