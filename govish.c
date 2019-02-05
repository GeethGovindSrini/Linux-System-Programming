/*This is a shell implemented by Geeth Govind S*/
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<string.h>
#include<signal.h>
#include<pwd.h>
#include<fcntl.h>
extern char **environ;
struct pipefd{
	int fds[2];
};
int last_occurence(char * s,char c){
	int i= 0,ind= 0;
	for(i= strlen(s);i>0;i--){
		if(s[i]== c)
			return i++;
	}
}
void print_prompt(){
	char *pwd= NULL;
	struct passwd *user;
	user= getpwuid(getuid());
	pwd= (char *) malloc (1000);
	getcwd(pwd,1000);				//Gets the value of the PWD from the environment variable
	printf("\x1b[32m");				//Prints in color green
	printf("GOVISH>>");
	printf("\x1b[0m");				//Prints in color white	
	printf(":");
	printf("\x1b[34m");				//Prints in color blue
	if(strstr(pwd,user->pw_dir)==NULL)
		printf("%s",pwd);
	else
		printf("~%s",&pwd[strlen(user->pw_dir)]);
	printf("\x1b[0m");				//Prints in color white
	printf("$ ");
	printf("\x1b[60m");				//Prints in color white
	fflush(NULL);
}
void clearscreen(char *argv[],char *envp[]){		//function to clear the screen
	int pid= fork(), status= -1;			//variable declarations and forking
	if(pid==0){					//if child
		execlp("clear","clear",NULL);		//execute clear
	}else{						//if parent
		wait(&status);				//wait till the child completes the process completes and
		pid= execvpe("govish",argv,envp);	//execute myself again
	}
}
void sigtstp_handler(int sig){
	printf("\b\b");
	fflush(NULL);
}
void sigint_handler(int sig){
	printf("\n");
	print_prompt();
}
void sigquit_handler(int sig){
	printf("\n");
	print_prompt();
}
char ** gettokens(char **tokens, int count){		//This method takes an input of the tokens and the command number and returns the command
	int bufsize= 10, position= 0, i= 0;
	char **token1 = malloc(bufsize * sizeof(char*));
	while(tokens[position]!=NULL && count!= 0){	//Moves the position to the required command
		if(*tokens[position]=='|')
			count--;
		position++;				
	}
	while(tokens[position]!= NULL && *tokens[position]!='|'){	//creates a new array of command for the requested number
		token1[i]= tokens[position];
		position++;
		i++;
		if (i>= bufsize) {
			bufsize+= 5;
			token1 = realloc(tokens, bufsize * sizeof(char*));
	      		if (!token1) {
			        perror("Govish");
		        	exit(1);
			}
    		}
	}
	if(!(i<(bufsize-2))){
		bufsize+= 1;
		token1 = realloc(tokens, bufsize * sizeof(char*));
		if (!token1){
		        perror("Govish");
		       	exit(1);
		}
	}
	token1[i]=NULL;
	return token1;					//returns the created array of the new command
}
void execpipecmd(char **tokens, int pipecount){		//This method recieves the array of commands including the pipes and executes them and produces the output as per requirement.
	int i= 0,j= 0,bufsize= 1024,pid= -1,parentfds[2], fds[2], status= 0, red= 0, exer= 0, fd= -1;
	char **tokens1= NULL, **tokens2=NULL, *out;
	char * args= malloc(bufsize * sizeof(char)),ch;
	struct pipefd *pipes= malloc (pipecount * sizeof(struct pipefd));	//Creating the required number of pipes
	int gpipe[2];					//pipe for synchronization
	pipe(gpipe);
	for(i=0;i<pipecount;i++){			//creating actual pipes
		if(pipe(pipes[i].fds)== -1)
			perror("Govish");
	}
	if(fork()== 0){					//creating children processes for executing pipes	
		for(i= 0;i<(pipecount+1);i++){		//loop to run the commands one by one
			tokens1= gettokens(tokens,i);	//getting the command to be executed
			if(fork()== 0){
				if(i== 0){		//if it's the first process
					dup2(pipes[i].fds[1],STDOUT_FILENO);
				}else if(i== pipecount){	//if it's the last process
					dup2(pipes[i-1].fds[0],STDIN_FILENO);
				}else{			//some process in the middle
					dup2(pipes[i-1].fds[0],STDIN_FILENO);
					dup2(pipes[i].fds[1],STDOUT_FILENO);
				}
				for(j=0;j<pipecount;j++){	//we have captured what we want. So closing the unwanted fds.
					close(pipes[j].fds[0]);
					close(pipes[j].fds[1]);
				}
				execvp(tokens1[0],tokens1);	//running the command given in the pipe
				perror("Govish");
				exit(0);
			}else{
				if(i>0)
					close(pipes[i-1].fds[0]);	//closing the read end of the previous process' pipe
				if(i>1)
					close(pipes[i-1].fds[1]);	//closing the write end of the previous process' pipe
				if(i== pipecount){
					close(pipes[i].fds[1]);		//close the read end of the current process' pipe
					close(pipes[i].fds[0]);		//close the write end of the current process' pipe
					wait(&status);			//wait for the last process to complete
					exit(0);
				}
			}
		}
	}else{	
		for(j=0;j<pipecount;j++){			//closing all unwanted fds
			close(pipes[j].fds[0]);
			close(pipes[j].fds[1]);
		}	
		close(gpipe[1]);				//closing the write end of the synchronization pipe
		while(read(gpipe[0],&ch,1)!=0);			//takes care of the synchronization
	}
}
int main(int argc, char *argv[],char *envp[]){
	char *line = NULL,*pwd= NULL,*pwd1= NULL,*dir= NULL,*last_occur= NULL,*temp= NULL, *cmd= NULL;
	int pid= -1, status= -1,dec= 0,bufsiz= 64,position= 0, flag= 0, pipecount= 0, fd= -1;
	char **tokens = malloc(bufsiz * sizeof(char*));
  	char *token, *var, *evar;
        ssize_t read;
	ssize_t bufsize = 0; 				// getline will allocate a buffer
	struct sigaction act;
	act.sa_handler= sigint_handler;
	act.sa_flags= 0;
	//sigaction(SIGINT,&act,NULL);
	signal(SIGINT,sigint_handler);			//signal handler for the Inturupt Signal
	signal(SIGTSTP,sigtstp_handler);		//signal handler for the t Stop Signal
	signal(SIGQUIT,sigquit_handler);		//signal handler for the t Stop Signal
	pwd= (char *) malloc (1000);
	//while(1){
	getcwd(pwd,1000);				//Gets the value of the PWD from the environment variable		
	if(argc!= 1){					//has some script to execute
		if(fork()== 0){
			fd= open(argv[1],O_RDONLY);	//opening that script
			dup2(fd,STDIN_FILENO);		//changing STDIN to the file
			read= getline(&line, &bufsize, stdin);	//ignore the first line
			while((read= getline(&line, &bufsize, stdin))>-1){	//loop to run all the commands one by one
				if(fork()== 0){
					cmd= strndup(line,strlen(line)-1);	//getting the command entered by the user removing the \n
						if((strstr(cmd,"--color")==NULL)/* && (strstr(cmd,"|")==NULL)*/)		//check if the user has entered '--color' in the command
							flag= 1;			//set the flag to 1 if it has been entered
						token = strtok(cmd, " ");		//tokenizing the command
					  	while (token != NULL) {		
							tokens[position] = token;	
							position++;
							if((strcmp(token,"ls")== 0) || (strcmp(token,"grep")== 0)){	//checking if the token is ls or grep
								if(flag){				//if the command does not have '--color'.
									tokens[position]= "--color";	//add the '--color' option to the command.
									position++;
									if (position >= bufsiz) {
										bufsiz++;
										tokens = realloc(tokens, bufsiz * sizeof(char*));
								      		if (!tokens) {
											perror("Govish");
											exit(1);
										}
							    		}
		
								}
								else			//if ls not found
									flag= 0;	//set flag to false
				
							}
							if (position >= bufsiz) {
								bufsiz+= 64;
								tokens = realloc(tokens, bufsiz * sizeof(char*));
						      		if (!tokens) {
									perror("Govish");
									exit(1);
								}
					    		}
					    		token = strtok(NULL, " ");
						}
						execvp(tokens[0],tokens);	//execute whatever is there in the file
						perror("Govish");
					
				}else{
					wait(&status);
				}
			}
		exit(0);
		}else{
			wait(&status);
			exit(0);
		}
		print_prompt();				//printing the Govish Prompt
	}else{
		print_prompt();				//printing the Govish Prompt
		dec= getchar();				//getting a char from the stdin
		if(dec== 12){				//if the character is ctrl+l
			clearscreen(argv,envp);		//clear the screen
		}else{					//if not ctrl+l
			dec= ungetc((unsigned char)dec, stdin);	//put back the character into the stdin
		}
		read= getline(&line, &bufsize, stdin);		//Gets the line entered in the terminal
		if(strstr(line,"\n")!=NULL && strlen(line)== 1){	//if the user has entered 'enter'
			dec= execvpe("govish",argv,envp);		//running myself again (Alternative for putting in while loop).
			if(dec== -1)					//Error Check
				perror("execvpe failed");		//execvpe failed
			//continue;
		}
		//printf("Retrieved line of length :%zu\n", read);
		//if(dec= strequals(line,"exit\n"))
		cmd= strndup(line,strlen(line)-1);	//getting the command entered by the user removing the \n
		if((strstr(cmd,"--color")==NULL)/* && (strstr(cmd,"|")==NULL)*/)		//check if the user has entered '--color' in the command
			flag= 1;			//set the flag to 1 if it has been entered
		token = strtok(cmd, " ");		//tokenizing the command
	  	while (token != NULL) {		
			if(strstr(token,"$")!=NULL){
				temp= strdup(token);
				var= strstr(temp,"$")+1;
				evar= getenv(var);
				if(evar!=NULL){
					var--;
					/*printf("Going to realloc\n");
					printf("size of temp:%d\n", strlen(temp));
					printf("size of var:%d\n", strlen(var));					
					printf("ENV Val:%s\n", evar);
					printf("size of evar:%d\n", strlen(evar));*/
					temp= realloc(temp, (strlen(temp)-strlen(var)+strlen(evar)) * sizeof(char));
					var= strstr(temp,"$");
					strcpy(var,evar);
					token= strdup(temp);
				}
				
			}
			tokens[position] = token;
			position++;
			if((strcmp(token,"ls")== 0) || (strcmp(token,"grep")== 0)){	//checking if the token is ls or grep
				if(flag){				//if the user has not entered '--color' in the command
					tokens[position]= "--color";	//add the '--color' option to the user command
					position++;
					if (position >= bufsiz) {
						bufsiz++;
						tokens = realloc(tokens, bufsiz * sizeof(char*));
				      		if (!tokens) {
							perror("Govish");
							exit(1);
						}
			    		}
		
				}
				else			//if not entered ls
					flag= 0;	//set flag to false
				
			}
			if (position >= bufsiz) {
				bufsiz+= 64;
				tokens = realloc(tokens, bufsiz * sizeof(char*));
		      		if (!tokens) {
					perror("Govish");
					exit(1);
				}
	    		}
	    		token = strtok(NULL, " ");
		}
	//	flag= 0; //to remove
	/*	if(flag){				//if the user has not entered '--color' in the command
			flag= 0;			//set back the flag to 0
			tokens[position]= "--color";	//add the '--color' option to the user command
			position++;
			if (position >= bufsiz) {
				bufsiz++;
				tokens = realloc(tokens, bufsiz * sizeof(char*));
		      		if (!tokens) {
					perror("Govish");
					exit(1);
				}
	    		}
		
		}*/
		tokens[position]= NULL;
		if(dec= strcmp(line,"exit\n")== 0){		//Checks if user typed 'exit'
			exit(0);				//exits the program with status 0
		}
		else if(strstr(line,"cd")!=NULL && strlen(line)== 3){
			dec= chdir(" ");
		}
		else if(strstr(line,"cd ")!=NULL){		//Checks if user typed 'cd'
			temp= strchr(line,' ')+1;		//Gets the string after 'cd'
			dir= strndup(temp,strlen(temp)-1);	//removing the '\n' at the end
			if(strcmp(dir,"..")== 0){		//if the user has entered '..' after cd
				last_occur= strrchr(pwd,'/');	//getting the address of the character '/'
				pwd1= strndup(pwd,last_occurence(pwd,'/'));	//removing the letters following it
				dec= chdir(pwd1);		//Changing the current working directory
				if(dec== -1)			//Error checking
					perror(argv[0]);
				else
					setenv("PWD",pwd1,1);		//setting the env variable PWD
			}else if(strcmp(dir,"/.")== 0){ 	//if the user has entered '/.' after cd
				setenv("PWD","/",1);		//set the environment variable to '/'
				dec= chdir("/");		//Changing the current working directory
				if(dec== -1)			//Error checking
					perror("Govish");
			}else {
				dec= chdir(dir);		//changing the current working directory
				if(dec== -1){			//Error checking
					perror("Govish");
				}
			}
		}
		else if(strstr(line,"|")!=NULL){		//if the given command contains a pipe in it.
			while(1){
				line= strchr(line,'|');
				if(line==NULL)
					break;
				pipecount++;			
				line++;
			}
			execpipecmd(tokens, pipecount);
		}
		else {						//we need to execute the command
			pid= fork();				//Creating another process for executing the user defined program
			if(pid==0){				//means child
				execvp(tokens[0],tokens);	//execute whatever the user has asked to
				perror("Govish");	
				exit(0);			//exit with the status 0
			}
		}
		wait(&status);					//wait till the child completes the process completes
		//}
		dec= execvpe("govish",argv,envp);		//running myself again (Alternative for putting in while loop).
		if(dec== -1)					//Error Check
			perror("execvpe failed");		//execvpe failed
	}	
	exit(0);
}
