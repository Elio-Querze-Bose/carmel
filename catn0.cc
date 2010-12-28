/*
  catn0: copy stdin to stdout - must be blocking FDs. requires linux 2.16.x or later for splice 0-copy; for TCP sockets:

  "Linux has had support for splicing from a TCP socket since 2.6.25 (commit 9c55e01c0), so if you're using an earlier version, you're out of luck"

  "In Linux 2.6.30.10 and older, splice returns EINVAL when the source or target filesystem does not support splicing. Here are the filesystems that do support splicing:

in read mode: adfs, affs, afs, bfs, btrfs, coda, ecryptfs, exofs, ext2, ext3, ext4, fat, fuse, hpfs, jffs2, jfs, minix, nfs, nilfs2, ntfs, ocfs2, omfs, qnx4, reiserfs, smbfs, sysv, ubifs, udf, ufs.
in write mode: exofs, ext2, ext3, ext4, jfs, ocfs2, reiserfs, ubifs.

Starting with Linux 2.6.31, all the filesystems support splicing both in read and write mode
"

  minimal library usage so as to fork/exec faster. build: gcc -O3 -std=c99 or g++ -O3

  optional arg1=N: exit after N bytes (N < 2^64). 0 = no limit.
  optional arg2=k: k=0 (default) exit w/ no error on stdin EOF or N bytes. k=1 exit w/ error code 2 on eof w/ less than N bytes read.
  optional arg3=s: timeout of s seconds (0=default means no timeout) - if no data received in last s seconds, exit with error code 3
*/

#define FALLBACK_TO_RW 1
// define to 0 for smaller binary that won't work with files on NFS (just TCP sockets and local files)

#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif
#include <fcntl.h> // splice

/*       long splice(int fd_in, off_t *off_in, int fd_out,
                   off_t *off_out, size_t len, unsigned int flags);
*/
const unsigned spliceflags=SPLICE_F_MOVE|SPLICE_F_MORE;
typedef long splice_size_t;
const splice_size_t largest_splice=256*1024; // xfer up to this much at a time if max isn't set. TODO: always limit to at least this small to help w/ buggy older linux?

#include <err.h> // err
#include <signal.h> // sigaction
#include <unistd.h> // alarm
#include <stdlib.h> //strtoull
#include <limits.h> // UINT_MAX
#include <errno.h>

// cp rfd->wfd (blocking), up to the first max bytes. return number of bytes written

/* a bug in kernel 2.6.31 that makes the first splice() call hang when you ask for all of the data on the socket. The Samba guys worked around this by simply limiting the data read from the socket to 16k

   - we don't do this; make sure you use a linux after that buggy version. */



unsigned timeout_handler_sec;
void timeout_handler(int signum) {
  errx(3,"catn: timed out after reading no data for %u sec",timeout_handler_sec);
}

typedef unsigned long long cat_size_t;


#if FALLBACK_TO_RW
// this is useless: doesn't actually check if you can splice unless bytes read/written >0. so fallback has to be able to handle read already happening but write can't splice.
bool can_splice(int fd1,int fd2) {
  if (splice(fd1,0,fd2,0,0,spliceflags)==-1)
    if (errno==EINVAL) return false;
  return true;
}
const cat_size_t default_bufsz=128*1024;

// use read/write as a fallback.
cat_size_t cat_fd_n(int rfd,int wfd,cat_size_t max,unsigned timeout_sec,cat_size_t bufsz=default_bufsz) {
  char buf[bufsz];
  cat_size_t totalw=0;
  ssize_t nr,nw;

  struct sigaction act,oldact;
  timeout_handler_sec=timeout_sec;
  if (timeout_sec) {
    act.sa_handler=timeout_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags=0;
    sigaction(SIGALRM,&act,&oldact);
  }

  for(;;) {
    if (max && totalw+bufsz>max) bufsz=max-totalw; // never read or write more than max if set.
    if (timeout_sec) alarm(timeout_sec);
    nr=read(rfd,buf,bufsz);
    if (timeout_sec) alarm(0);
    if (nr==-1) err(4,"cat_fd_n failed read from fd %d",rfd);
    if (nr==0) break; //EOF
    totalw+=nr;
    char *bufend=buf+nr;
    for (;nr;nr-=nw)
      if ((nw=write(wfd,bufend-nr,nr))==-1 || nw==0)
        err(5,"cat_fd_n failed write to fd %d",wfd);
    if (max && totalw>=max) break;
  }
  if (timeout_sec) sigaction(SIGALRM,&oldact,0); // restore old handler
  return totalw;
}

#endif

cat_size_t cat_fd_n_splice(int rfd,int wfd,size_t max,unsigned timeout_sec,size_t max_bytes) {
  int pipefd[2]; // man 2 pipe - kernel buffer for 0 copy splice
  if (pipe(pipefd)==-1)
    err(6,"catn0 failed to create pipe() for splice kernel buffer");

  cat_size_t n_xfered=0; // for fallback

  if (FALLBACK_TO_RW) {
    goto start;
    fallback:
      warn("catn0: falling back to read/write because of no support for splice on reader or writer e.g. NFS file");
//      if (timeout_sec) alarm(0);
      close(pipefd[0]);close(pipefd[1]);
      return n_xfered+cat_fd_n(rfd,wfd,max,timeout_sec);
  start:
    bool use_rw=false;
    if (!can_splice(rfd,pipefd[1])) {
      warn("can't splice from fd %d; fallback to read/write",rfd);
      use_rw=true;
    }
    if (!can_splice(pipefd[0],wfd)) {
      warn("can't splice to fd %d; fallback to read/write",wfd);
      use_rw=true;
    }
    if (use_rw) {
      goto fallback;
    }
  }

  cat_size_t remain=max?max:largest_splice;
  splice_size_t nr,nw;

  struct sigaction act,oldact;
  timeout_handler_sec=timeout_sec;
  if (timeout_sec) {
    act.sa_handler=timeout_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags=0;
    sigaction(SIGALRM,&act,&oldact);
  }

  for(;;) {
    if (timeout_sec) alarm(timeout_sec);
    nr=splice(rfd, 0, pipefd[1], 0, remain>largest_splice?largest_splice:remain, spliceflags);
    if (timeout_sec) alarm(0);
    if (nr==-1) {
      if (FALLBACK_TO_RW && errno==EINVAL) {
        goto fallback;
      } else {
//      if (errno==EINTR) continue;
// we don't allow any signals (that don't kill us entirely; EAGAIN is for nonblocking only // ||errno==EAGAIN
        err(4,"catn0 failed splice from fd %d to pipe %d",rfd,pipefd[1]);
      }
    }

    if (nr==0) break; //EOF
    if (max) remain-=nr;
    n_xfered+=nr;

    for (;nr;nr-=nw) {
      nw=splice(pipefd[0],0,wfd,0,nr,spliceflags);
      if (nw==0)
        errx(5,"catn0 splice returned 0 bytes written to fd %d",wfd);
      if (nw==-1) {
        if (FALLBACK_TO_RW && errno==EINVAL) {
          char buf[nr];
          if (read(pipefd[0],buf,nr)!=nr) err(7,"catn0: while falling back to write instead of splice, couldn't get bytes that were stored in kernel buffer (read already succeeded)");
          char *bufend=buf+nr;
          for (;nr;nr-=nw)
            if ((nw=write(wfd,bufend-nr,nr))==-1 || nw==0)
              err(7,"catn0: while falling back to write instead of splice, failed writing the bytes we got (read already succeeded)");
          goto fallback;
        } else {
          //if (nw==-1&&errno==EINTR) continue; // we don't allow interrupts! die!
          err(5,"catn0 failed splice from pipe %d to fd %d",pipefd[0],wfd);
        }

      }
    }
    if (remain==0) break;
  }
  if (timeout_sec) sigaction(SIGALRM,&oldact,0); // restore old handler
  close(pipefd[0]);close(pipefd[1]);
  return n_xfered;
}

// err unless whole string is used (no whitespace allowed)
unsigned long long ull_or_die(char const* c,int argi,unsigned long long max,char const* name) {
  char *end;
  errno=0;
  unsigned long long i=strtoull(c,&end,0);
  char const *errmsg="arg %d - wanted unsigned %s but got '%s'";
  if (*end || !*c) errx(1,errmsg,argi,name,c);
  if (errno) err(1,errmsg,argi,name,c); // should just be ERANGE in C99
  if (max && i>max) errx(1,"arg %d (%s) too large - max allowed is %llu",argi,name,max);
  return i;
}

int main(int argc, char *argv[]) {
  cat_size_t max=0;
  int err_incomplete=0;
  unsigned timeout_sec=0;

  //arg parsing:
  int ai=0;
  if (++ai<argc) {
    max=ull_or_die(argv[ai],ai,0,"max-length");
    if (max==0) return 0; // else we'll use max==0 to indicate no limit
  }
  if (++ai<argc)
    if (argv[ai][0]=='1') err_incomplete=1;
  if (++ai<argc)
    timeout_sec=ull_or_die(argv[ai],ai,UINT_MAX,"timeout-sec");

  //action:
  cat_size_t nout=cat_fd_n_splice(STDIN_FILENO,STDOUT_FILENO,timeout_sec,max);
  if (err_incomplete && nout<max)
    return 2;

  return 0;
}
