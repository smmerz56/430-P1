#include <stdio.h> 
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <stdbool.h> 
#include <fcntl.h>
#include <stdlib.h>

//******************************************************************************
//Shell.c
//takes user input and executes the commands 
//
//
//*************************Notes for use:
//  -If the command does require the parent to wait(no &), please wait for 
//   the osh> promt
//
//  -If the command doesnt require the parent to wait(& was entered), the osh> 
//   prompts formatting will be off. Enter the next command even if no osh>
//******************************************************************************

#define MAX_LINE 80 /* The maximum length command */

bool loadArgs(char temp[], char *token, char *args[], int arrayCell, char *history[], char *fileName[], int *redirectType, char *pipeArgs[]);
void resetArray(char *array[]);
void getHistory(char *history[], char *args[], int *redirectType, char *fileName[], char* pipeArgs[]);
void redirectCode1(char *fileName[]);
void redirectCode2(char *fileName[]);
void redirectCode3(char *history[], char* pipeArgs[]);

int main()
{
  char *args[MAX_LINE/2 + 1]; /* command line arguments */
  int should_run = 1; /* flag to determine when to exit program */ 
  char temp[20]; //temp char array to store user input
  char *token = NULL;  // char pointer used by strtok in loadArgs()
  char *history[MAX_LINE/2 + 1]; // stores previous args
  char *fileName[1];// stores file name
  fileName[0] = NULL;
  int redirectType = 0;// if command is <, >, |
  char *pipeArgs[MAX_LINE/2 + 1];// the second argument after |
  
  for(int i = 0; i < MAX_LINE/2 + 1; i++){// inititalize arrays
  
    args[i] = NULL;
    history[i] = NULL;
    pipeArgs[i] = NULL; 
    
    if(i < 20){
    
      temp[i] = 0;
      
    }
  }

  while (should_run) {

    int arrayCell = 0; //cell in args[]
   
    printf("osh>");
    fflush(stdout);
    
    // loads the args array, and returns whether fork() parent 
    bool runConcurently = loadArgs(temp, token, args, arrayCell, history, fileName, &redirectType, pipeArgs);
    
    if(args[0] != NULL){
      if(strcmp(args[0], "exit") != 0){ // if command is not "exit"
      
        int pid = fork();
        
        if(pid < 0){
          printf("Error: fork() failed!"); 
          exit(1);
        }
        else if(pid == 0){ //child
                   
          if(strcmp(args[0], "!!") == 0){ 
          
            getHistory(history, args, &redirectType, fileName, pipeArgs);
            
          }
          else{ 
            
            if(redirectType != 3){ // skip this code, need to pipe, but stay in child
            
              if(fileName[0]) { // if there is something in file[]
                                            
                if(redirectType == 1){ // >, redirect output
                
                  redirectCode1(fileName);
                                          
                }
                if(redirectType == 2){ // <, redirect input
                
                  redirectCode2(fileName);
                        
                }                   
              }
              
              int bad = execvp(args[0], args);// execute command
                    
              if(bad < 0){
              
                printf("Invalid Command!\n");
                resetArray(args);
                resetArray(history);
                exit(1);
                
              }
            }
            else{//RedirectType == 3, Pipe
             
              int pipefd[2];
              int pid2 = 0;
              
              if(pipe(pipefd) < 0){
              
                printf("Pipe Error");
                exit(1);
                
              }
              
              pid2 = fork();
            
              if(pid2 < 0){
              
                printf("Fork Error");
                exit(1);
                
              }     
                       
              if(pid2 == 0){// child
               
                close(pipefd[1]);                
                dup2(pipefd[0],0);                         
                execvp(pipeArgs[0], pipeArgs);
                
                exit(0);
              }
              else{//parent-child
              
               close(pipefd[0]);
               dup2(pipefd[1], 1);
               execvp(args[0], args);
               
               exit(0);              
              }
            }
          }
        }
        else{ //parent
          
          if(!runConcurently){ //no &
         
            int status;          
            wait(&status);
            sleep(1);// added to help with the foramtting of the osh after a & was in a command
           
          }                 
        }
      }
      else{ // if command is exit, end executing shell
    
        should_run = 0; 
        
      }
    }
      
    resetArray(args); 
   
  }
 
  resetArray(history);// before program exits, release memory from history[]
  
  return 0; 
}

//******************************************************************************
// loadArgs
// takes input from user, loads into args[] and history[], returns is parent should wait or not 
bool loadArgs(char temp[], char *token, char *args[], int arrayCell, char *history[], char *fileName[], int *redirectType, char* pipeArgs[]){ 
  bool answer = false;
   
  fgets(temp, 20, stdin);   
  
  for(int i = 0; i < 20; i++){// look for & and \n, and remove them
  
    if (temp[i] == '\n'){
    
      temp[i] = 0;
            
    }
    if(temp[i] == '&'){
    
      temp[i] = 0;
      answer =  true;
      
    }   
  } 
  
  token = strtok(temp," "); // get first token
    
  if(token != NULL){ // if there is no token (just \n entered), skip all this code
  
    if(strcmp(token, "!!") != 0){ //skip if history command
      
      resetArray(history); //free memories
      resetArray(pipeArgs);
      *redirectType = 0;
      
      if(fileName[0] != 0){ //free memory
      
        free(fileName[0]);
        fileName[0] = NULL;
        
      }
      
      while(token != NULL){ //while there is a token to parse
      
        if(strcmp(token, ">") == 0){
          
          token  = strtok(NULL, " ");// move on to next token           
          fileName[0] = malloc(strlen(token) +1);
          strcpy(fileName[0], token);
           
          *redirectType = 1;// mark as a >
          token = NULL; //exit while loop
         
        }
        else if(strcmp(token, "<") == 0){
          
          token  = strtok(NULL, " ");// move on to next token          
          fileName[0] = malloc(strlen(token) +1);
          strcpy(fileName[0], token);
          
          *redirectType = 2;// mark as a <
          token = NULL; //exit while loop
          
        }
        else if(strcmp(token, "|") == 0){
          
          token  = strtok(NULL, " ");// move on to next token         
          pipeArgs[0] = malloc(strlen(token) +1);
          strcpy(pipeArgs[0], token);
          
          *redirectType = 3;// mark as a |
          token = NULL;//exit while loop
          
        }
        else{// a command without <, >, | 
         
          args[arrayCell] = malloc(strlen(token) +1);// load arg into args[]
          strcpy(args[arrayCell], token); 
          
          history[arrayCell] = malloc(strlen(token) +1);// load arg into history[]
          strcpy(history[arrayCell], token);
         
          token = strtok(NULL, " ");// move on to next token
          arrayCell++;
          
        }
      }
    }
    else{// A history call, set args[] to !!
      
     
      args[arrayCell] = malloc(strlen(token) +1);//keep
      strcpy(args[arrayCell], token);
     
    } 
  }
 
  return answer;// return whether user entered a & or not
}


//******************************************************************************
// resetArray
// free array memory
void resetArray(char *array[]){
  for(int i = 0; i < MAX_LINE/2 + 1; i++){ //reset args[] need
     if(array[i] != 0){
      
       free(array[i]);
           
     }
     
      array[i] = NULL;
  } 
}

//******************************************************************************
// getHistory
// if a !! is entered
void getHistory(char *history[], char *args[], int *redirectType, char *fileName[], char* pipeArgs[]){
  
  if(history[0] != NULL){// if !! was the first command, skip this code, go to else
  
    int cell = 0;
    printf("osh>");
    
    while(history[cell] != 0){// print previous command
      
      printf(history[cell++]); 
     
    }
    
    if(*redirectType == 1){// previous command had a >
    
     printf(" > %s \n", fileName[0]);
     redirectCode1(fileName);
     
    }
    else if(*redirectType == 2){// previous command had a <
    
     printf(" < %s \n", fileName[0]);
     redirectCode2(fileName);
     
    }
    else if(*redirectType == 3){// previous command had a |
    
     printf(" | %s \n", pipeArgs[0]);
     redirectCode3(history, pipeArgs);
     
    }
    else{// formatting
    
     printf("\n");
     
    }
  
    int bad = execvp(history[0], history);// exec previous command
    
    if(bad < 0){
    
      printf("Invalid Command!\n");
      resetArray(args);
      resetArray(history);
      exit(1);
      
    }
  }
  else{// if !! was entered as the first command
  
    printf("No commands in history");
    printf("\n"); 
    resetArray(args);
    exit(1);
    
  }
  
}

//******************************************************************************
// redirectCode1
// code used in main and getHistory for file redirection if > 
void redirectCode1(char *fileName[]){

  int error = 0;
  int redirect = 0;
  
  redirect = open(fileName[0], O_CREAT | O_WRONLY | O_TRUNC, 0644);
  error = dup2(redirect, 1);
  
  if(error < 0){
  
    printf("dup2 error!\n");
    exit(1);
    
  }
  
  close(redirect);
}

//******************************************************************************
// redirectCode2
// code used in main and getHistory for file redirection if <
void redirectCode2(char *fileName[]){
  
  int redirect = open(fileName[0], O_RDONLY);
  int error = dup2(redirect, 0);
  
  if(error < 0){
  
    printf("dup2 error!\n");
    exit(1);
    
  }
  
  close(redirect);
}

//******************************************************************************
// redirectCode2
// code used in getHistory with piping
void redirectCode3(char *history[], char* pipeArgs[]){

  int pipefd[2];
  int pid2 = 0;
  
  if(pipe(pipefd) < 0){
  
    printf("Pipe Error");
    exit(1);
    
  }
  
  pid2 = fork();

  if(pid2 < 0){
  
    printf("Fork Error");
    exit(1);
    
  }     
           
  if(pid2 == 0){// child
   
    close(pipefd[1]);                
    dup2(pipefd[0],0);           
    execvp(pipeArgs[0], pipeArgs);
    exit(0);
    
  }
  else{//parent-child
  
   close(pipefd[0]);
   dup2(pipefd[1], 1);
   execvp(history[0], history);
   exit(0); 
                
  }
             
}