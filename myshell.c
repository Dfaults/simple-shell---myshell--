/*
Title: Final Shell
name: Dfaults

*/ 

#include "myshell.h" 

int main (int argc, char *argv[])
{
	char buf[MAX_BUFFER], pwd[MAX_ARGS];    // line buffer
	char shell_path[MAX_ARGS]="shell=";
	char readme_path[MAX_ARGS]="readme_path=";
	char newpath[MAX_ARGS*1000];
	int len;  
	strcpy(newpath,getenv("PATH"));
	strcat(newpath,":");
	if(strcmp(argv[0],"./myshell")&&strcmp(argv[0],"myshell")) 
	{                 
		len=strlen(argv[0]);  
		while(len&&argv[0][len]!='/')
		 len-- ; 
		argv[0][len]='\0';           
		strcpy(pwd,argv[0]);    
		get_fullpath(pwd, argv[0]);  
		printf("%s\n",pwd);  
	}
	else
		strcpy(pwd,getenv("PWD"));    

	strcat(newpath,pwd);   // strcat(newpath,getenv("PWD"));     
	setenv("PATH",newpath,1);// add the current working directory  in the "PATH" environment variable to search for the filename specified.
	strcat(shell_path,pwd);   //  strcat(shell_path, getenv("PWD"));
	strcat(shell_path,"/myshell");
	putenv(shell_path); //add the working directory  of myshell in the environment variables           
	strcat(readme_path, pwd);      
	strcat(readme_path,"/readme");         
	putenv(readme_path); //   add the filepath of the file "readme"  in the  environment variables, see function  my_help( ) ! 
	
	if(argc>1)  // User input directly from the terminal  ./myshell a.bat  >c.txt 
	{
		strcpy(buf,"myshell ");
		int i;
		for(i=1;i<argc;i++)
		{
			strcat(buf,argv[i]);
			strcat(buf,"  ");
		}
		Execute(buf);// execute this command(bat)£¬  
	}
	
	else  // if user input ./myshell
	{
		Command_clear( );
		fprintf(stderr, "Welcome to the shell\nType \"help\" to see the help information.\n");          
		Command_shell(stdin,NULL,NULL);
	}
    return 0 ; 
}    // end  function "main" 


