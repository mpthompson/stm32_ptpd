// This file stubs out newlib interfaces for GCC.  It is not needed for ARMCC.
#if !defined(__ARMCC_VERSION) && defined (__GNUC__)

#include <errno.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/unistd.h>

#undef errno
extern int errno;

// Minimal empty environment.
char *__env[1] = { 0 };
char **environ = __env;

void __attribute__((weak)) _exit(int status)
{
  for (;;);
}

int __attribute__((weak)) _close(int file)
{
  return -1;
}

int __attribute__((weak)) _execve(char *name, char **argv, char **env)
{
  errno = ENOMEM;
  return -1;
}

int __attribute__((weak)) _fork()
{
  errno = EAGAIN;
  return -1;
}

int __attribute__((weak)) _fstat(int file, struct stat *st)
{
  st->st_mode = S_IFCHR;
  return 0;
}

int __attribute__((weak)) _getpid()
{
  return 1;
}

int __attribute__((weak)) _isatty(int file)
{
  switch (file)
  {
    case STDOUT_FILENO:
    case STDERR_FILENO:
    case STDIN_FILENO:
      return 1;
    default:
      //errno = ENOTTY;
      errno = EBADF;
      return 0;
  }
}

int __attribute__((weak)) _kill(int pid, int sig)
{
  errno = EINVAL;
  return -1;
}

int __attribute__((weak)) _link(char *old, char *new)
{
  errno = EMLINK;
  return -1;
}

int __attribute__((weak)) _lseek(int file, int ptr, int dir)
{
  return 0;
}

caddr_t __attribute__((weak)) _sbrk(int incr)
{
  errno = ENOMEM;
  return  (caddr_t) -1;
}

int __attribute__((weak)) _read(int file, char *ptr, int len)
{
  errno = EBADF;
  return -1;
}

int __attribute__((weak)) _stat(const char *filepath, struct stat *st)
{
  st->st_mode = S_IFCHR;
  return 0;
}

clock_t __attribute__((weak)) _times(struct tms *buf)
{
  return -1;
}

int __attribute__((weak)) _unlink(char *name)
{
  errno = ENOENT;
  return -1;
}

int __attribute__((weak)) _wait(int *status)
{
  errno = ECHILD;
  return -1;
}

int __attribute__((weak)) _write(int file, char *ptr, int len)
{
  errno = EBADF;
  return -1;
}

void exit(int reason)
{
  (void)(reason);
  while(1);
}

#endif
