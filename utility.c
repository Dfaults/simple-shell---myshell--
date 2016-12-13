#include "myshell.h"


int back_bat=0; //  it means it is both background process and batch, effective value of 1
int output_num=0; //  the number of output batch number in redirection
char batchfile[MAX_PATH] ; //  The current batch file
int bat_num=0;// Number of batch command-line
int isbat=0;// symbol of whether a batch
int letter; 
char *open; 


/*  Function "Execute" : used as the function system( ), it interpret the input, and call function "Command_exec" to  execute the all the commands.  */
int Execute(char *buffer)
{
	pid_t pid;
	char *args[MAX_ARGS];  /// pointers to arg strings
	int error ;
	int states[5]; // states[0] is back exec; states[1] is inputfile num; states[2] is outputfile num ;
	// states[3] is priority of inputfile(not args); states[4] is argc ;
	Redirect  Inputs [MAX_OPEN];  //   input    redirection (10)
	Redirect  Outputs[MAX_OPEN]; //   output   redirection   (10)

	error=Command_strtok(buffer,args,states,Inputs,Outputs);// tokenize  input, last  entry  will  be  NULL
	if(error||args[0]==NULL) return -1; // If the input format error	or  if there's anything there

	if ( !strcmp(args[0],"quit") || !strcmp(args[0],"exit"))   // "quit" command
	{
		if(args[1])   //   no argument is needed after "quit"
			Error(-2,args+1,NULL,NULL,args[0]); ///

		if(output_num>1)// e.g.  myshell test.bat >m.txt >n.txt
		{
			fprintf(stderr,"Exit\n");
			return 1;
		}

		if(isbat)   // /
			fprintf(stderr,"Batch file \"%s\" is finished!\n",batchfile);
		else fprintf(stderr,"\nGoodbye\n\n");
		exit (0);      // break out of 'while (!feof(stdin))' loop in "main" function
	}

	else if(states[0])// states[0]==1  running in background
	{
		switch (pid=fork( ))
		{
		case -1:
			Error(-9,NULL,NULL,states,"fork");
		case  0:  // child   //sleep(1);
			Command_delay(12);
			fprintf(stderr,"\n");
			Command_exec(args,Inputs,Outputs,states);
			exit(1);
		default:
			if(isbat==0) //
				fprintf(stderr,"pid=%d\n",pid);
		}// end switch
	}

	else // states[0]==0  running in front
		Command_exec(args,Inputs,Outputs,states);

	return 0;
}


/*  Function "Command_exec"  : execute the command   */
int Command_exec(char **args, const Redirect *Inputs,const Redirect *Outputs,int *states)
{
	char filepath[MAX_PATH] , parent[MAX_ARGS] ;
	FILE * outputfile=NULL,* inputfile;
	pid_t newpid;
	int flag ;

	if (!strcmp(args[0],"myshell")||!strcmp(args[0],"shell"))   // "myshell" command
	{
		flag=0;
		if(isbat)//     e.g.   execute myshell b.txt in test.bat
		{
			switch(newpid=fork( ))
			{
			case -1:
				Error(-9,NULL,NULL,states,"fork");
			case 0:
				if(states[0] &&(args[1]|| states[1]))
				{
					back_bat++;
					flag=1;
				}
				output_num=states[2];
				Command_bat(args,Inputs,Outputs,states);   //
				if(flag)
					back_bat--;
				output_num=0;
				exit (0);
			default:
				waitpid(newpid, NULL, WUNTRACED);
			}
		}
		else
		{
			if(states[0]&&(args[1]|| states[1]))
			{
				back_bat++;
				flag=1;
			}
			output_num=states[2]; /////////
			Command_bat(args,Inputs,Outputs,states);       //////
			if(flag)
				back_bat--;
			output_num=0;////////////
		}
		if(states[0])
			exit(1);
		else
			return 0;
	}

	if(states[2])// set output Redirection : use freopen()
	{
		get_fullpath(filepath, Outputs->filename);
		outputfile=freopen(filepath, Outputs->opentype,stdout);
		if(outputfile==NULL)
		{
			Error(-6,NULL,NULL,NULL,Outputs->filename);
			if(states[0])
				exit(1);
			else
				return -4;
		}
	}

	// check for internal/external command
	if (!strcmp(args[0],"cd")) // "cd "  command
		Command_cd(args,Inputs,states);

	else if (!strcmp(args[0],"clr")||!strcmp(args[0],"clear")) // "clear"  command
	{       // system("clear");
		if(output_num==0 )
			Command_clear( );  // In Command_clear()，execute clear
		if(args[1]|| states[1]||states[2])     //  no argument is needed after "clear"
			Error(4,NULL,NULL,NULL,args[0]);
	}

	else if (!strcmp(args[0],"dir")) // "dir"  command
		Command_dir(args,Inputs,states);

	else if (!strcmp(args[0],"echo"))  // "echo" command
		Command_echo(args,Inputs,states); // 

	else if (!strcmp(args[0],"environ"))  // "environ" command
	{
		list_environ( );
		if(states[1])//   invalid input redirection        e.g.   environ   <m.txt
			Error(-3,NULL,Inputs,states,"environ");
		if(args[1])    // no argument is needed after "pwd"
			Error(-2,args+1,NULL,NULL,"environ");
	}

	else if (!strcmp(args[0],"help")||!strcmp(args[0],"?"))   // "help" command
	{
        Command_help(args,Outputs,states); ///////////////////////////
		if(states[1])    // no argument is needed after "pwd"
			Error(-3,NULL,Inputs,states,args[0]);
	}

	else if (!strcmp(args[0],"pause"))   // "pause" command
	{
		if(args[1]|| states[1]|| states[2])     //  no argument is needed after "pause"
			Error(4,NULL,NULL,NULL,args[0]);
		if(states[0]+back_bat==0) //                                        ////////////////////////////////
			getpass("Paused\npress <Enter> key to continue");
	}

	else  if (!strcmp(args[0],"pwd"))   // "pwd" command
	{
		show_pwd(  );
		if(states[1])//   invalid input redirection        e.g.   environ   <m.txt
			Error(-3,NULL,Inputs,states,"pwd");
		if(args[1])    // no argument is needed after "pwd"
			Error(-2,args+1,NULL,NULL,"pwd");
	}

	// check for external excutive file,  e.g.    ls
	else
	{
		strcpy(parent,"parent=") ;
		strcat(parent, getenv("shell"));
		switch (newpid=fork( ))
		{
		case -1:
			Error(-9,NULL,NULL,NULL,"fork");
		case 0:   // execution in child process
			if(states[1])// input redirection
			{
				get_fullpath(filepath,Inputs->filename);
				inputfile= freopen(filepath,"r",stdin); // open file
				if(inputfile==NULL)
				{
					Error(-6,NULL,NULL,NULL,Inputs->filename);
					exit(1);
				}
			}
			putenv( parent); //
			execvp(args[0],args);
			Error(-1,args,NULL,NULL,NULL);//
			if(inputfile)
				fclose(inputfile);
			exit(0);
		default:
			if(states[0]==0 )waitpid(newpid, NULL, WUNTRACED);
		} // end  'switch'
	}  //end  ' else'

	if(outputfile)
	{
		fclose(outputfile);
		freopen("/dev/tty","w",stdout);
	}

	if(states[2]>1)// more output
	{
		states[2]--;////
		Command_exec (args, Inputs, Outputs+1,states) ;///////
	}

	if(states[0]) exit(0);
	else return 0;
}//  end  function   Command_exec( )


/*   Function "Error"  :    used to report errors   */
int Error(int errortype,char **args,const Redirect *  IOputs,const int *states, char * msg) //
{
	int i;
	if(isbat)  // if executes from batchfile
		fprintf(stderr,"Line %d of inputfile \"%s\": ",bat_num,batchfile);
	switch(errortype)
	{
	case  0:
		fprintf(stderr," %s\n",msg);  //
		break;
	case  1:
		fprintf(stderr,"Format Error: invlid argument '%s'(Letter %d), without openfile\n",open,letter);// <  >>  >
		fprintf(stderr,"\nType 'help redirection'  to get help information abbout '<'  '>>' and '>'\n");
		break;
	case  2:
		fprintf(stderr,"Format Error(Letter %d): '%s' followed by invlid argument '%c'\n", letter,open,*msg); // <  >> >
		fprintf(stderr,"\nType 'help redirection'  to get help information abbout '<'  '>>' and '>'\n");
		break;

	case 3:
        fprintf(stderr,"Sorry: no help information about \"%s\" found.\n",msg);
		fprintf(stderr,"\nType \"man %s\" to search in the man pages of the system.\nNote: press the key <Q> to exit the search.\n",msg);
		break;
	case 4:
        fprintf(stderr,"Note: no argument is needed after \"%s\", except the background-execute flag '&' .",msg);   //
		fprintf(stderr,"\nType 'help %s' to get usage abbout '%s'.\n",msg,msg);
		break;

	case 5:
		fprintf(stderr,"\nSystem Note: can not open more than %d files as %s\n",MAX_OPEN,msg);
		break;

	case -1:
		fprintf(stderr,"\nSystem Warning: \"");
		while (*args)
			fprintf(stderr,"%s ",*args++);
		fprintf(stderr,"\b\" is not internal command or executive file\n");
		if(isbat==0&&output_num==0)//
			fprintf(stderr,"\nType \"help command\" to see supported internal commands.\n");
		break; //abort( );

	case -2:
		fprintf(stderr,"\nFormat Warning: invalid arguments \"" ) ;
		while(*args)
			fprintf(stderr,"%s ",*args++);
		fprintf(stderr,"\b\" after command \"%s\" \n",msg);
		break;

	case -3:
		fprintf(stderr,"Invalid input redirection: ");
		for(i=0;i<states[1];i++)
			fprintf(stderr,"\"<%s\" ",IOputs[i].filename);
		fprintf(stderr,"after \"%s\" !\n",msg);
		break;

	case -4:   fprintf(stderr,"Invalid output redirection: ");
		for(i=0;i<states[1];i++)
			fprintf(stderr,"\"%s%s\" ",IOputs[i].opentype,IOputs[i].filename);
		fprintf(stderr,"after \"%s\" \n",msg);
		break;

	case -5:
		fprintf(stderr,"Path Error: \"%s\": not a directory or not exist\n",msg);
		break;

	case -6:
		fprintf(stderr,"File Error: can not open file \"%s\"\n",msg);
		break;

	case -7:
		fprintf(stderr,"Overflow Error: the assigned dirpath is longer than permitted longth(%d)\n",MAX_PATH);
		break;

	default:
		fprintf(stderr,"%s: %s\n", strerror(errno), msg);
		break;
		abort( );
	}
	return 1;
}


/*   Function "Command_strtok" :  analyse the command
<1> record the input command line(stored in buf[]) into args[],
<2> record background execute,argc,etc in states[]
<3> record the input files and output files */
int Command_strtok(char *buf,char **args,int *states,Redirect *Inputs,Redirect *Outputs) // redirection of InPut & OutPuts
{
	int i,j,n,m, flag,argc,errortype;   // flag is the symbol of blank；argc is the number of parameters(not include redirection and background process）
	char c ;
	states[0]=states[1]=states[2]=states[3]=states[4]=0;// states[0] is the symbol of background process;states[1] number of input redirection;
	//states[2] the number of output redirection；states[3]number of parameters； states[4] label input redirection before keybroad input；
	errortype=letter=argc=0;
	args[0]=NULL;
	open=NULL;// the method to open；
	flag=1;
	i=m=n=-1;
	while(buf[++i]&&buf[i]!='#')    // scan the command, "#" for remark
	{
		c=buf[i];
		switch(c)             //  Character Analysis
		{
		case '<':
			letter++;
			if(flag==0)
			{
				flag=1;
				buf[i]='\0';
			}
			open="<"; // "r" opentype
			while(buf[++i]==' '|| buf[i]=='\t'); // Skip a number of consecutive spaces

			if(buf[i]<32|| buf[i]=='#' ) // invlid argument '<', without inputfile!
			{
				errortype=Error(1,NULL,NULL,NULL,"<");//     invlid argument '<', without inputfile!
				break;
			}
			else if(buf[i]=='&'|| buf[i]=='<'|| buf[i]=='>'|| buf[i]=='|'|| buf[i]==';') // can not  be    <&    <;   <<   <>     <|
			{
				letter++;
				errortype=Error(2,NULL,NULL,NULL,buf+i);
				break;
			}

			if(argc<2)
				states[4]=1;
			m++;
			i--;
			break;

		case '>':
			letter++;
			if(flag==0)
			{
				flag=1;
				buf[i]='\0';
			}
			n++;
			if(buf[i+1]=='>')
			{
				buf[++i]='\0';
				open=">>"; // "a" opentype
			}
			else
				open=">";  //  "w" opentype

			while(buf[++i]==' '|| buf[i]=='\t'); 	// Skip a number of consecutive spaces

			if(buf[i]<32|| buf[i]=='#' )
			{
				errortype=Error(1,NULL,NULL,NULL,NULL);
				break;
			}
			else if(buf[i]=='&'|| buf[i]=='<'|| buf[i]=='>'|| buf[i]=='|' || buf[i]==';') 	// can not  be    >&    <;   ><     <|
			{
				letter++;
				errortype=Error(2,NULL,NULL,NULL,buf+i);
				break;
			}
			i--;
			break;

		case '&':
			letter++;
			if(flag==0)
			{
				flag=1;
				buf[i]='\0';
			}
			if(states[0])
			{
				errortype= Error(0,NULL,NULL,NULL,"Format Error: argument '&' occurs more than once");
				break;
			}
			states[0]=1;
			break;

		case ' ':
		case '\t':
			if(flag==0)
			{
				flag=1;
				buf[i]='\0';
			}
			while(buf[++i]==' '|| buf[i]=='\t');  // Skip a number of consecutive spaces
			i--;
			break;

		case '\n':
		case '\r':
			buf[i]='\0';
			i--;
			break;

		default:
			letter++;
			if(flag)
			{
				flag=0;
				if(open&&m<=MAX_OPEN&&n<=MAX_OPEN)
				{
					if(m==MAX_OPEN)// too many input redirections
						errortype=Error(5,NULL,NULL,NULL,"input");
					else  if(n==MAX_OPEN)// too many  output redirections
						errortype=Error(5,NULL,NULL,NULL,"output");
					else if( !strcmp(open,"<" ) )
						Inputs[m].filename=buf+i;
					else  if( !strcmp(open,">>" ) )
					{
						strcpy(Outputs[n].opentype,"a");
						strcpy(Outputs[n].open,">>");
						Outputs[n].filename=buf+i;
					}
					else  if( !strcmp(open,">" ) )
					{
						strcpy(Outputs[n].opentype,"w");
						strcpy(Outputs[n].open,">");
						Outputs[n].filename=buf+i;
					}
					open=NULL;
				}
				else args[argc++]=buf+i;
			}
			if(c=='\\'  &&buf[i+1]==' ')   //  Escape characters  e.g.   "a\ b"   means  "a b"  in  filename or directoryname
			{
				buf[i]=' ';
				if( ! isspace(buf[i+2]))
				{
					j=i+1;
					while(  buf[++j])  buf[j-1]=buf[j];
				}
			}
		} // end switch
	 } // end for loop
	 args[argc]=NULL; // args end with NULL
	 states[1]=m+1;//   0,12,...,m
	 states[2]=n+1;//  0,1,2,...,n
	 states[3]=argc;
	 if(errortype||(argc==0&&letter))     // if there's anything there
		 Error(0,NULL,NULL,NULL,"Warning: nothing will be executed");//
	 return errortype;  // errortype==0 is ok ;
}


/*   Function "Command_cd" : execute the command "cd"    */
int Command_cd (char **args,const Redirect *Inputs,int *states)//
{
    char dirpath[MAX_PATH], filepath[MAX_PATH], dirname[MAX_PATH];	  //='\0';
    char  *current_dir;
	int i,flag;
	FILE *inputfile;
	if(states[4])
	{
		if(args[1])///invalid arguments
			Error(-2,args+1,NULL,NULL,"cd");
		if(--states[1])
			Error(-3,NULL,Inputs+1,states,"cd");

		get_fullpath(filepath,Inputs->filename);
		inputfile=fopen(filepath,"r");  //  open file
		if(inputfile==NULL)//  can not open
		{    
			Error(-6,NULL,NULL,states,Inputs->filename);
			return -2;   // File Error
		}

		fgets(dirname,MAX_PATH,inputfile);   ///
		fclose(inputfile);
		args[1] = strtok(dirname," \b\n\r");////
		i=2;
		while (( args[i] = strtok(NULL," \b\n\r") ) )  i++;
	}

	else  if (states[1])   //  invalid input redirection       e.g.   cd  /home <a.txt     or   cd  <a.txt <b.txt
		Error(-3,NULL,Inputs,states,"cd");

	if(args[1])   //the argument of pathname is given
	{
	       if(args[2])   // more than one argument
			   Error(-2,args+2,NULL,NULL,"cd");
		   get_fullpath(dirpath,args[1]);
	} //end  " if(args[1]) "

	else 	{
		fprintf(stdout,"%s\n",getenv("PWD"));
		return 0;
	}

	flag=chdir(dirpath);   //Change the working directory
	if(flag)	//If the change fails
	{
		Error(-5,NULL,NULL,states, args[1]);
		return -2;
	}
	//If the directory changed successfully, modify the value of PWD
	current_dir=(char *)malloc(MAX_BUFFER);//allocate  memory space
	if(!current_dir)        //if fails to allocate memory space
		Error(-9,NULL,NULL,states,"malloc failed");
	getcwd(current_dir,MAX_BUFFER);  //get current working directory
	setenv("PWD",current_dir,1);     //modify the value of PWD
	free(current_dir);
	return 0;
}    // end function "Command_cd"



/*   Function "Command_clear " : execute the command "clear"    */
void Command_clear(void)
{      // system("clear");
	pid_t newpid;
	switch (newpid=fork( ))
	{
	case -1:
		Error(-9,NULL,NULL,NULL,"fork");
	case 0:   // execution in child process
		execlp("clear","",NULL); //
		Error(-9,NULL,NULL,NULL,"execlp"); // error if return from exec

	default:      waitpid(newpid, NULL, WUNTRACED);
		fprintf(stderr,"\nCleared\n\n");/////////////////////////////
	}// end switch
	return;
}    // end function "Command_clear"


/*   Function "Command_dir " : execute the command "dir"    */
int Command_dir(char **args,const Redirect *Inputs, int *states)//
{
	FILE  *inputfile ;
	   pid_t newpid;
       DIR   *pdir;
       int   i;
       char   filepath[MAX_PATH], dirpath[MAX_PATH], dirname[MAX_PATH];
	   /*    execute the directory   */
	   if(states[4])
	   {
		   if(args[1])///invalid arguments
			   Error(-2,args+1,NULL,NULL,"dir");
		   if(--states[1])
			   Error(-3,NULL,Inputs,states,"dir");

		   get_fullpath(filepath,Inputs->filename);
    	      inputfile=fopen(filepath,"r");  //  open file
			  if(inputfile==NULL)//  can not open
			  {    Error(-6,NULL,NULL,states,Inputs->filename);
			  return -2;   // File Error
			  }
			  fgets(dirname,MAX_PATH,inputfile);   //
			  fclose(inputfile);
			  args[1] = strtok(dirname," \b\n\r");//
			  i=2;
			  while (( args[i] = strtok(NULL," \b\n\r") ) )  i++;
	   }

	   else  if (states[1])   //  invalid input redirection       e.g.   cd  /home <a.txt
		   Error(-3,NULL,Inputs,states,"dir");

	   if(args[1])   //the argument of pathname is given
	   {
		   if(args[2])   // more than one argument
			   Error(-2,args+2,NULL,NULL,"dir");
		   get_fullpath(dirpath,args[1]);
	   } //end  " if(args[1]) "

       else strcpy(dirpath, "."); //  just   "dir"   means   " dir . "
       /* check if the directory exists */
	   pdir=opendir(dirpath);
	   if(pdir==NULL)	//if nonexist
	   {
		   Error(-5,NULL,NULL,states,args[1]);
		   return -2 ;
	   }
	   /* execute dir comman */
	   switch (newpid=fork( ))
	   {
	   case -1:
		   Error(-9,NULL,NULL,states,"fork");
	   case 0:       // execution in child process
		   execlp( "ls","ls" ,"-al", dirpath, NULL);     ///  ls
		   Error(-9,NULL,NULL,states,"execlp"); // error if return from exec
	   default:   waitpid(newpid, NULL, WUNTRACED);
	   }    // end switch
	   return 0;
}    // end  function  "Command_dir"


/*   Function "Command_echo " : execute the command "echo"    */
int Command_echo (char **args,const Redirect *Inputs,int *states)//
{
	FILE * inputfile;
	char filepath[MAX_PATH];
	char buf[MAX_BUFFER];
	int j,k;
	if(states[4])  // input redirection is before args[1] ,   e.g.     echo  <a.txt  hello
	{
		if(args[1])    // args[1] is invalid
			Error(-2,args+1,NULL,NULL,"echo");
		for(j=0;j<states[1];j++) // e.g.  echo <a.txt <b.txt
		{
			get_fullpath(filepath,Inputs[j].filename);
			inputfile=fopen(filepath,"r");
			if(inputfile==NULL)
			{
				Error(-6,NULL,NULL,NULL,Inputs[j].filename);
				return -2;
			}
			if(states[2]==0&&output_num==0)//  no output file  is open
				fprintf(stderr,"The contents of file \"%s\" is:\n",Inputs[j].filename);
			while (!feof(inputfile))    // display  the  contents  of  file
			{
				if(fgets(buf, MAX_BUFFER, inputfile))  //
					fprintf(stdout,"%s",buf);
			}	//
			fclose(inputfile);
			fprintf(stdout,"\n");
		}
	}

   else
   {
	   if(states[1])//   invalid input redirection        e.g.    echo hello   <m.txt
	   Error(-3,NULL,Inputs,states,"echo");
	   if(args[1])
	   {
		   for(k=1;k<states[3]-1;k++)
			   fprintf(stdout,"%s ",args[k]);//fputs(args[k],outputfile);
		   fprintf(stdout,"%s",args[k]);
	   }
	   fprintf(stdout,"\n");
   }
   return 0;
}    // end  function  "Command_echo"


/*   Function "list_environ" : execute the command "environ"  */
int list_environ (void)
{
	char ** env = environ;
	while(*env) fprintf(stdout,"%s\n",*env++);
	return 0;
}


/*   Function "show_pwd" : execute the command "pwd"  */
int show_pwd (void)//
{
	fprintf(stdout,"PWD=%s\n",getenv("PWD"));
	return 0;
}

/*  Function "Command_shell" : keep reading a line from stdin or inputfile and call "Execute()" to execute the command line.  */
int Command_shell(FILE *inputfile,const Redirect *Outputs,const int *states) //
{
	FILE *outputfile;
	char filepath[MAX_PATH];
	char buf[MAX_BUFFER];
    int done=0;  //

	if(Outputs)
	{
		get_fullpath(filepath,Outputs->filename);
		outputfile=freopen(filepath,Outputs->opentype,stdout);
		if(outputfile==NULL)
		{
			Error(-6,NULL,NULL,NULL,Outputs->filename);
			return -2;///
		}
		fprintf(stderr,"\nThe results will be writen into file \"%s\".\n",Outputs->filename);
	}
	bat_num=0;
	do    // /* keep reading input until "quit" command or eof of redirected input */
	{        /* get command line from input */
		if (inputfile==stdin&&Outputs==NULL)  ///////////////////////////////
			fprintf(stderr,"\n[%s@%s]$: ",getenv("USERNAME"),getenv("PWD"));  // write prompt
		if (fgets(buf, MAX_BUFFER, inputfile))  // read  a  line
		{
			bat_num++;
			done=Execute(buf);    //
			if(done==1) // "quit" command is executed
			{
				if(Outputs)
					freopen("/dev/tty", "w", stdout);
				break;//exit(0);
			}
		}
	}
	while (!feof(inputfile));  //  do ... while( );

	if(Outputs)
		freopen("/dev/tty", "w", stdout);
	return 0;
}


/*   Function "Command_delay" : delay to ensure the order of processes   */
void Command_delay(int n)
{
	n=n*1000000;
	while(n--) ;
}



/*   Function "get_fullpath" : get the full path of a file or a directory   */
void get_fullpath(char *fullpath,const char *shortpath)  //
{
	int i,j;
	i=j=0;
	fullpath[0]=0;
	char *old_dir, *current_dir;

	if(shortpath[0]=='~')// e.g.  ~/os
	{
		strcpy(fullpath, getenv("HOME"));
		j=strlen(fullpath);
		i=1;
	}

	else  if(shortpath[0]=='.'&&shortpath[1]=='.')// e.g.  ../os
	{
		old_dir=getenv("PWD");
		chdir("..");
		current_dir=(char *)malloc(MAX_BUFFER);//allocate memory space
		if(!current_dir)        //if allocate fails
			Error(-9,NULL,NULL,NULL,"malloc failed");

		getcwd(current_dir,MAX_BUFFER);  //get current working directory
		strcpy(fullpath, current_dir);
		j=strlen(fullpath);
		i=2;
		chdir(old_dir);
	}
	else   if(shortpath[0]=='.')// e.g.   ./os
	{
		strcpy(fullpath, getenv("PWD"));
		j=strlen(fullpath);
		i=1;
	}
	else if(shortpath[0]!='/')// e.g.   os/project1
	{
		strcpy(fullpath, getenv("PWD"));
		strcat(fullpath, "/");
		j=strlen(fullpath);
		i=0;
	}
	strcat(fullpath+j,shortpath+i);
	return;
}


/*  Function "Command_help" : display the user manual;    seek  for  the key word such as  <help  dir> , output the file "readme" until meeting '#'*/
int Command_help(char **args,const Redirect *Outputs,int *states)//
{
	FILE *readme;
	char buffer[MAX_BUFFER];
	char keywords [MAX_BUFFER]="<help ";
	int i,len;
	for(i=1;args[i];i++)
	{
		strcat(keywords,args[i]);
		strcat(keywords," ");
	}
	len=strlen(keywords);
	keywords[len-1]='>';
	keywords[len]='\0';

	if(!strcmp(keywords,"<help more>")) // "help more "  means showing the whole user manual!
	{
		strcpy(buffer,"more ");
		strcat(buffer,getenv("readme_path"));
		for(i=0;i<states[2];i++)
		{
			strcat(buffer,Outputs[i].open);
			strcat(buffer,Outputs[i].filename);
		}
		Execute(buffer);     //
		return 0;
	}

	readme=fopen(getenv("readme_path"),"r"); // while execute myshell，add readme_path into the environment variables in initialization phase

	while(!feof(readme)&&fgets(buffer,MAX_BUFFER,readme))  // looking for keywords   such  as   <help dir>
	{
		if(strstr(buffer,keywords))
		break;
	}
	while(!feof(readme)&&fgets(buffer,MAX_BUFFER,readme))//   display  from here   until meet   '#'
	{
		if(buffer[0]=='#')
			break;
		fputs(buffer,stdout);    // display help information
	}

	if(feof(readme))// if not found the key words
	{
		keywords[len-1]='\0';   //  key words is  such as  "<help dir>"
		Error(3,NULL,NULL,NULL,&keywords[6]);
	}
	if(readme)
		fclose(readme);
	return 0;
}


/*   Function "Command_bat" : execute the command "myshell" with a batchfile    */
int Command_bat(char **args,const Redirect *Inputs,const Redirect *Outputs, int *states)//
{	  //   fprintf(stderr,"mybat");
	FILE *inputfile;  //
	char filepath[MAX_PATH];
	int i=0;
	char fullpath_batchfile[MAX_PATH] ;  //  fullpath of pervious batch file  e.g  execute myshell test.bat, test.bat include the line: myshell b.bat
	//  fullpath_batchfile records the directory of test.bat，in case of dead circle  e.g  excute  myshell test.bat, test.bat includes the line: myshell test.bat
	pid_t  newpid;
	if(isbat)  // if executes from batchfile
		fprintf(stderr,"Line %d of inputfile \"%s\": ",bat_num,batchfile);//
	if(states[4])  // e.g.  myshell <test.bat      ==>     myshell test.bat
	{
		if(args[1])// e.g.   myshell <test.bat b.bat
			fprintf(stderr,"Note: can not open more than one inputfile after commnad '%s'.\n",args[0]);
		args[1]=Inputs->filename;
		--states[1];
		i=1;
	}
	if(args[1])  // e.g.  myshell test.bat    myshell <a.bxt
	{
		if(states[1]>0)  //  e.g.    myshell a.txt <b.txt
			Error(-3,NULL,Inputs+i,states,args[0]);  //  invalid  input redirection
		if(args[2])   // more than one argument   e.g.    myshell  a.txt  b.txt
			Error(-2,args+2,NULL,NULL,args[0]);

		get_fullpath(filepath,args[1]);
		get_fullpath( fullpath_batchfile,batchfile) ;  //  fullpath of batch file
		if(isbat && !strcmp(fullpath_batchfile,filepath))// e.g.  myshell a.txt, a.txt has the command line of myshell a.txt
		{
			fprintf(stderr,"Warning: commands not execute, it will cause infinite loop\n");
			return -5;
		}

		inputfile=fopen(filepath,"r");    //  open input redirection file
		if(inputfile==NULL)// can not open  file !
		{
			Error(-6,NULL,NULL,NULL,args[1]);
			return -2;
		}

		isbat=1; 	  //
		strcpy(batchfile,args[1]);
		fprintf(stderr,"Turn to execute the commands in batch file \"%s\":\n",batchfile);

		if( ! output_num)//
			Command_shell(inputfile,NULL,states);  //read command lines from Command_shell()，analyse and execute，put the result to OutPuts

		else
			for(i=0;i<states[2];i++)//
			{
				Command_shell(inputfile,Outputs+i,states);///
				rewind(inputfile);///
				output_num--;///
			}
			fclose(inputfile);
			fprintf(stderr,"\nExecution of batch file \"%s\" is finished\n",batchfile);
			bat_num=isbat=0;    //
	}

	else   if( output_num) //e.g.  myshell >b.txt >n.txt
	{
		isbat=0; //
		switch(newpid=fork( ))
		{
		case -1:
			Error(-9,NULL,NULL,states,"fork");
		case 0:
			fprintf(stderr,"Please type commands, the results will be writen into \"%s\":\n",Outputs->filename);
			Command_shell(stdin,Outputs,states); //
			exit(0);
		default:
			waitpid(newpid, NULL, WUNTRACED);
		}

		if( output_num>1) //
		{
			fprintf(stderr,"\n");
			output_num--;
			Command_bat(args,Inputs,Outputs+1,states);
		}
	}

	else//Shell Producers: version 5.2
		fprintf(stdout,"\nProduced by: Dfaults\n");

	return 0;
}


