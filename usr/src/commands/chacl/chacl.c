/* chacl - Change file's Access Control List modes				Author: V. Archer, Andrew Hill */

/* Copyright 1991 by Vincent Archer
 *	You may freely redistribute this software, in source or binary
 *	form, provided that you do not alter this copyright mention in any
 *	way.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <minix/minlib.h>
#include <stdio.h>
#include <lib.h>

#include <pwd.h>
#include <grp.h>

#ifndef S_ISLNK
#define S_ISLNK(mode)	0
#define lstat		stat
#endif

#define USR_MODES (S_ISUID|S_IRWXU)
#define GRP_MODES (S_ISGID|S_IRWXG)
#define EXE_MODES (S_IXUSR|S_IXGRP|S_IXOTH)
#ifdef S_ISVTX
#define ALL_MODES (USR_MODES|GRP_MODES|S_IRWXO|S_ISVTX)
#else
#define ALL_MODES (USR_MODES|GRP_MODES|S_IRWXO)
#endif


/* Common variables */
typedef char * string;

_PROTOTYPE(void usage, (void));
_PROTOTYPE(void usage1, (void));
_PROTOTYPE(void usage2, (void));
_PROTOTYPE(void usage3, (void));

/* Main module. The single option possible (-R) does not warrant a call to
 * the getopt() stuff.
 */
int main(int argc, char *argv[])
{

/* 	Parse Format
*	chacl -[ug] name [+-][RWX] filenames(s)
*/
  int ex_code = 0;
  char *name;
  int type, add, perms, i = 0;
  char **filearray;
  int numStrings, numFiles;
  numStrings = 10;
  //strWidth = 100;
  filearray = (char **)calloc(numStrings, sizeof(char*));
  message m;

  
  
  argc--;
  argv++;

  if(strncmp(*argv,"-u", 2) == 0) type = 1;
  else if(strncmp(*argv,"-g", 2) == 0) type = 0;
  else usage();
  
  argc--;
  argv++;
  
  name = *argv;
  
  argc--;
  argv++;
   
  if(strncmp(*argv, "-", 1) == 0) add = 0;
  else if (strncmp(*argv, "+", 1) == 0) add = 1;
  else usage1();
 
  
  if(strncmp(*argv, "+RWX", 4) == 0) perms = 0x17;
  else if(strncmp(*argv, "+RW", 3) == 0) perms = 0x16;
  else if(strncmp(*argv, "+WX", 3) == 0) perms = 0x13;
  else if(strncmp(*argv, "+RX", 3) == 0) perms = 0x15;
  else if(strncmp(*argv, "+R", 2) == 0) perms = 0x14;
  else if(strncmp(*argv, "+W", 2) == 0) perms = 0x12;
  else if(strncmp(*argv, "+X", 2) == 0) perms = 0x11;
  else if(strncmp(*argv, "-RWX", 4) == 0) perms = 0x7;
  else if(strncmp(*argv, "-RW", 3) == 0) perms = 0x6;
  else if(strncmp(*argv, "-WX", 3) == 0) perms = 0x3;
  else if(strncmp(*argv, "-RX", 3) == 0) perms = 0x5;
  else if(strncmp(*argv, "-R", 2) == 0) perms = 0x4;
  else if(strncmp(*argv, "-W", 2) == 0) perms = 0x2;
  else if(strncmp(*argv, "-X", 2) == 0) perms = 0x1;
  else usage2();
  
  argc--;
  argv++;
  
  numFiles = argc;
  
  for( ; argc > 0; argc--) {
	if(argc > 10) {usage3(); break;}
	filearray[i] = *argv;
	i++;
	argv++;
	//&filearray++;
  }

  printf("chacl %s ", name);
  printf("%d ", type);
  printf("%d %d ", add, perms);
  for(int j = 0; j < 10; j++) {
	printf("%s ", filearray[j]);
  }
  printf("\n");
  
  printf("test \n");
  
  char *ACL;
  printf("test \n");
  ACL = "/etc/ACL";
    printf("test \n");
  
  
  m.m7_i1 = type;
  m.m7_i2 = perms;
  m.m7_p2 = ACL;
  m.m7_i5 = strlen(ACL)+1;
   printf("test \n");


 uid_t userId;		/* effective user id */
 gid_t groupId;		/* effective group id */
 
 if(type == 0) {
 groupId = getgrnam(name)->gr_gid;
 m.m7_i3 = groupId;
 }
 else if(type == 1) {
 userId = getpwnam(name)->pw_uid;
 m.m7_i3 = userId;
 }

  printf("test \n");

  for(int k = 0; k < numFiles; k++) {
	if(filearray[k] == NULL) {printf("loop break: %d \n", k); break; }
	else {
	printf("loop: %d \n", k);
	printf("%s %d \n", filearray[k], strlen(filearray[k]));
	m.m7_p1 = filearray[k];
	m.m7_i4 = strlen(filearray[k])+1;
	//strcpy(m.m3_ca1, filearray[k]);
	//printf("%s \n", m.m3_ca1);
	_syscall(VFS_PROC_NR, 70, &m);
	}
  }
 
  
  return(ex_code);
}



  
  
  

/* Display Posix prototype */
void usage()
{
  std_err("Usage: chacl incorrect usage at -[ug]...\n");
  exit(1);
}
void usage1()
{
  std_err("Usage: chacl incorrect usage at add...\n");
  exit(1);
}
void usage2()
{
  std_err("Usage: chacl incorrect usage at RWX...  Make sure to use upper case RWX letters. \n");
  exit(1);
}
void usage3()
{
  std_err("Usage: You are entering too many files at one time. \n");
  exit(1);
}
