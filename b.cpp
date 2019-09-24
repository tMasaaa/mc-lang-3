extern "C" {
    int fib(int);
}
extern "C" int MyMain(unsigned long (*edk_print)(const char16_t* format, ...) __attribute__((ms_abi))) {
  edk_print(u"fib(45) = %lu\n", fib(45));
  return 0;
}
