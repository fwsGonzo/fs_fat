inline void panic(const char* why)
{
  printf("***\n");
  printf("*** PANIC: %s\n", why);
  printf("***\n");
  abort();
}
