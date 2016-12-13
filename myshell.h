#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <dirent.h> 
#include <ctype.h>


#define MAX_BUFFER 1024  // max line buffer
#define MAX_ARGS 64  // max # args
#define SEPARATORS " \t\n"  // token sparators
#define NUM 10        // the command number
#define MAX_OPEN 10  //  open 10 stdin redirection files max and 10 stdout redirection files, myshell support several I/O redirection£©
#define MAX_PATH 100 //  the maxium length of file and forder 



/***********************************************************************
announce certain data types
***********************************************************************/
typedef struct// redirection date structure
{ 
	char *filename;   // redirection file name
	char opentype[3]; // the open method of redirection files "a"   "r"   "w" 
	char open[3];  // the open method of redirection files  ">>"   "<"   ">" 
} Redirect;          //

extern int errno;        // system error number
extern char **environ;   // environment array


/***********************************************************************
announce the functions
***********************************************************************/

int Execute(char *buffer);//the execution of the command
int Command_exec(char **args, const Redirect *Inputs,const Redirect *Outputs,int *states);//
int Error(int errortype,char **args,const Redirect *  IOputs,const int *states, char * msg) ;//error message printout
int Command_strtok(char *buf,char **args,int *states,Redirect *InPuts,Redirect *OutPuts);//analyse the command
int Command_cd(char **args,const Redirect *Inputs, int *states);// execute command 'cd' 
void Command_clear(void);// execute command 'clear' 
int Command_dir(char **args,const Redirect *Inputs, int *states);// execute command 'dir'  
int Command_echo(char **args,const Redirect *Inputs,int *states);// execute command 'echo' 
int list_environ(void);// execute the command "environ" 
int show_pwd(void); // execute command 'pwd' 
int Command_shell(FILE *inputfile,const Redirect *Outputs,const int *states);//keep reading a line from stdin or inputfile and call "Execute()" to execute the command line.
void Command_delay(int n);           //delay to ensure the order of processes
void get_fullpath(char *fullpath,const char *shortpath);  //get the full path of a file or a directory
int Command_help(char **args,const Redirect *Outputs,int *states);// display the user manual;    seek  for  the key word such as  <help  dir> , output the file "readme" until meeting  '#'
int Command_bat(char **args,const Redirect *Inputs,const Redirect *Outputs,int *states); // execute the command "myshell" with a batchfile 
