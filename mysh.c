#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>


struct AliasListItem{
   char* alias_name;
   char* replacement_string;
   struct AliasListItem* next;
};

int main(int argc, char* argv[]){
  size_t nread;
  char* line = NULL;  // Pointer to MEM where a line will be read to
    // Note: Create MACROS or hardcode?
  char const* exit_str = "exit";
  //int const exit_len = strlen(exit_str);
  struct AliasListItem* AliasList=NULL;
  struct AliasListItem* AliasListIterator=NULL;
  struct AliasListItem* DelItem=NULL;

  // Feature 1: Interactive  Mode
//  if ((argc == 1)) {
    char const *prompt = "mysh> ";
    int prompt_len = strlen(prompt); // NOTE: MAC / HARD CODE
    int interactive;

    if(argc>2){
     write(STDERR_FILENO,"Usage: mysh [batch-file]\n",strlen("Usage: mysh [batch-file]\n"));
     exit(1);
    }

    interactive = (argc==1)?1:0;
    FILE* batch_fp=NULL;
    
    batch_fp = (interactive)?NULL:fopen(argv[1],"r");

    //char str1[1024]; strcpy(str1,"Error: Cannot open file "); 
    //char str2[8]; strcpy(str2,".\n");

    if(!interactive && batch_fp == NULL){
     //strcat(str1,argv[1]);
     //strcat(str1,".\n");
     write(STDERR_FILENO,"Error: Cannot open file ",strlen("Error: Cannot open file "));
     write(STDERR_FILENO,argv[1],strlen(argv[1]));
     write(STDERR_FILENO,".\n",strlen(".\n"));
     exit(1);
    }
    //while(1){
      if(interactive)
       write(STDOUT_FILENO, prompt, prompt_len);  // Write the prompt "mysh> " to the user
      while((nread = getline(&line, &nread, interactive?stdin:batch_fp)) != -1){ // Write command line input to MEM pointed to by line
      // Code to create new process for each command (down)
        
        char *token;
        char *linedup;
        char *linedup_bkp;
        char *line_arg;
	char **command_parse;
	int command_count,i,j,k,l;
        char *delimiter = " ";
	int alias_comm = 0;

	if(interactive==0)
	 write(STDOUT_FILENO,line,strlen(line));

	line[strlen(line)-1] = '\0';
        int z=0; int y=0;
	while(line[y]){
         if(!isspace(line[y]))
	  z=1;
	 y++;
	}
	if(z==1){
       
        int isspace_before = 0;
	int isspace_after = 0;
	int isspace_both = 0;
	int isspace_none = 0;
	int redir_location;
	i=0;
	j=0;
	while(line[i]){
         if(i==0 && !isspace(line[i])){
          j++; 
	 }
	 else if(isspace(line[i]) && !isspace(line[i+1]) && line[i+1] != '\0'){
	  j++;
	 }

         if(i>0){
	  if(line[i] == '>' && isspace(line[i-1]) && !isspace(line[i+1])){
	   isspace_before = 1;
	   redir_location = j-1;
	  }
	  if(line[i] == '>' && isspace(line[i-1]) && isspace(line[i+1])){
	   isspace_both = 1;
	   redir_location = j-2;
	  }
	  if(line[i]=='>' && isspace(line[i+1]) && !isspace(line[i-1])){
	   isspace_after = 1;
	   redir_location = j-1;
	  }
	  if(line[i]=='>' && !isspace(line[i+1]) && !isspace(line[i-1])){
	   isspace_none = 1;
	   redir_location = j-1;
	  }
	 }

	 i++;
	}

        command_count = j;
        command_parse = (char**)malloc((command_count+1)*sizeof(char*));

        i=0; j=0;
	char redir_file[512];
	int exec_program = 1;
	int prog = 0, redirection = 0, file = 0, file_done = 0;
	while(line[i]){
         if(!isspace(line[i]) && line[i]!='>') prog = 1;
	 if(redirection==1 && line[i] == '>') exec_program = 0;
	 if(redirection && !isspace(line[i])) file = 1;
	 if(line[i] == '>') redirection = 1;
	 if(file == 1 && (isspace(line[i]) || line[i] == '\0')) file_done = 1;

         if(prog==0 && redirection==1) exec_program = 0;
	 if(file_done && !isspace(line[i])) exec_program = 0;

	 if(file && !file_done){
          redir_file[j] = line[i];
	  j++;
	 }

	 i++;
	}
	redir_file[j] = '\0';

	if(redirection && file == 0) exec_program = 0;

	if(exec_program == 0){
         write(STDERR_FILENO,"Redirection misformatted.\n",strlen("Redirection misformatted.\n"));
	 //continue;
	}

        i=0;
	j=0;
	k=0;
	l=0;
        while(line[i]){
         if(i==0 && !isspace(line[i])){
	  j=i;
          while(!isspace(line[j]) && line[j] != '\0'){
           j++;
	  }
	  command_parse[l] = (char*)malloc((j+1)*sizeof(char));
	  j=i;
	  while(!isspace(line[j]) && line[j] != '\0'){
           command_parse[l][j] = line[j];
	   j++;
          }
	  command_parse[l][j] = '\0';
	  l++;
	  i = j;
	 }
	 else if(isspace(line[i]) && !isspace(line[i+1]) && line[i+1] != '\0'){
	  j=i+1;
	  k=0;
	  while(!isspace(line[j]) && line[j] != '\0'){
           k++; j++;
	  }
	  command_parse[l] = (char*)malloc((k+1)*sizeof(char));
	  j=i+1;
	  k=0;
	  while(!isspace(line[j]) && line[j] != '\0'){
	   command_parse[l][k] = line[j]; 
           j++; k++;
	  }
	  command_parse[l][k] = '\0';
	  l++;
          i=j;
	 }
	 else
	  i++;
	}
	command_parse[l] = NULL;

        if(redirection && exec_program){
         if(isspace_before){
	  for(i=redir_location+1;command_parse[i];i++)
	   free(command_parse[i]);
	  free(command_parse[i]);
	  free(command_parse[redir_location]);
	  command_parse[redir_location] = NULL;
	 }

	 if(isspace_after)
          command_parse[redir_location][strlen(command_parse[redir_location])-1] = '\0';

	 if(isspace_none)
          for(i=0;command_parse[redir_location][i];i++){
	   if(command_parse[redir_location][i] == '>'){
	    command_parse[redir_location][i] = '\0';
            break;
	   }
	  }
	 if(isspace_both||isspace_after){
	  for(i=redir_location+2;command_parse[i];i++)
	   free(command_parse[i]);
	  free(command_parse[i]);
	  free(command_parse[redir_location+1]);
          command_parse[redir_location+1] = NULL;   
	 }
	}

	i=0;
	j=0;
	AliasListIterator = AliasList;
	while(AliasListIterator){
	 if(strcmp(AliasListIterator->alias_name,command_parse[0])==0){
          j=1;
	  linedup = (char*)malloc((strlen(AliasListIterator->replacement_string)+1)*sizeof(char));
	  linedup_bkp = (char*)malloc((strlen(AliasListIterator->replacement_string)+1)*sizeof(char));
	  strcpy(linedup,AliasListIterator->replacement_string);
	  strcpy(linedup_bkp,AliasListIterator->replacement_string);
	 }
         AliasListIterator = AliasListIterator->next;
	}
	if(j==1){
 	 token = strtok(linedup_bkp,delimiter);
	 command_count = 0;
	 while(token != NULL){
            token = strtok(NULL,delimiter);
	    command_count++;
	 }
	 for(i=0;command_parse[i];i++)
	  free(command_parse[i]);
	 free(command_parse[i]);
	 free(command_parse);
	 free(linedup_bkp);
	 command_parse = (char**)malloc((command_count+1)*sizeof(char*));
         token = strtok(linedup, delimiter);

         i=0;
         while(token != NULL){
	     command_parse[i] = token;
	     i++;
             token = strtok(NULL, delimiter);
           }
	 command_parse[i] = NULL;
	 alias_comm = 1;
        }
	line_arg = (char*)malloc((strlen(command_parse[0])+1)*sizeof(char));
	strcpy(line_arg,command_parse[0]);
        if(strcmp(line, exit_str) == 0){
	    free(line_arg);
        for(i=0; command_parse[i] ; i++)
	 free(command_parse[i]);
	free(command_parse[i]);
	free(command_parse);
	free(line);
	if(interactive==0)
	 fclose(batch_fp);
	int flag =0;
	AliasListIterator = AliasList;
	while(AliasListIterator){
         free(AliasListIterator->alias_name);
	 free(AliasListIterator->replacement_string);
	 if(DelItem != NULL)
	  free(DelItem);
	 DelItem = AliasListIterator;
	 AliasListIterator = AliasListIterator->next;
	 flag = 1;
	}
	if(flag) free(DelItem);
            exit(0);
        }

        //Executing Commands 
        if(exec_program){ //Check if the command is valid, lexical check and check for exit also, this entire if needs to be inside both interactive/batch mode

        if(strcmp(command_parse[0],"alias")==0){
         if(command_count == 1){ //Print entire list
          AliasListIterator = AliasList;
          while(AliasListIterator){
           write(STDOUT_FILENO,AliasListIterator->alias_name,strlen(AliasListIterator->alias_name)*sizeof(char)); 
           write(STDOUT_FILENO," ",sizeof(char));
           write(STDOUT_FILENO,AliasListIterator->replacement_string,strlen(AliasListIterator->replacement_string)*sizeof(char)); 
           write(STDOUT_FILENO,"\n",sizeof(char));
           AliasListIterator = AliasListIterator->next;
          }
         }
         else if (command_count == 2){ // Print for that alias name if exists
          AliasListIterator = AliasList;
          while(AliasListIterator){
           if(strcmp(AliasListIterator->alias_name,command_parse[1])==0){
            write(STDOUT_FILENO,AliasListIterator->alias_name,strlen(AliasListIterator->alias_name)*sizeof(char));
            write(STDOUT_FILENO," ",sizeof(char));
            write(STDOUT_FILENO,AliasListIterator->replacement_string,strlen(AliasListIterator->replacement_string)*sizeof(char)); 
            write(STDOUT_FILENO,"\n",sizeof(char));
           }
           AliasListIterator = AliasListIterator->next;
          }
         }
         else if (command_count >= 3){ //Add alias
          if(strcmp(command_parse[1],"alias")==0 || strcmp(command_parse[1],"unalias")==0 || strcmp(command_parse[1],"exit")==0){//Screw this saying "alias: Too dangerous to alias this"
           write(STDERR_FILENO,"alias: Too dangerous to alias that.\n",strlen("alias: Too dangerous to alias that.\n"));
          }
          else{
           struct AliasListItem* AliasListReplace = NULL;
           if(AliasList==NULL) AliasListIterator = AliasList;
           else{
            AliasListIterator = AliasList;
            while(AliasListIterator){
             if(strcmp(AliasListIterator->alias_name,command_parse[1])==0) AliasListReplace=AliasListIterator;
             AliasListIterator = AliasListIterator->next;
            }
           }
           if(AliasListReplace == NULL){
            AliasListIterator = (struct AliasListItem*)malloc(sizeof(struct AliasListItem));
            AliasListIterator->next = NULL;
            AliasListIterator->alias_name = (char*)malloc((strlen(command_parse[1])+1)*sizeof(char));
            strcpy(AliasListIterator->alias_name,command_parse[1]);
           }
           int mem_length=0;
           for(i=2;command_parse[i];i++)
            mem_length += strlen(command_parse[i]);
           mem_length += command_count - 2 //For space after each command
                         + 1; //For endline
           if(AliasListReplace == NULL){
            AliasListIterator->replacement_string = (char*)malloc(mem_length*sizeof(char));
            for(i=2,k=0; command_parse[i]; i++){
             j=0;
             while(command_parse[i][j]){
              AliasListIterator->replacement_string[k] = command_parse[i][j];
              k++;
              j++;
             }
             AliasListIterator->replacement_string[k] = (i==(command_count-1))?'\0':' ';
             k++;
            }
	    struct AliasListItem* AliasListItemLocal = NULL;
	    if(AliasList == NULL) AliasList = AliasListIterator;
	    else{
             AliasListItemLocal = AliasList;
	     while(AliasListItemLocal){
              if(AliasListItemLocal->next == NULL){
	       AliasListItemLocal->next = AliasListIterator;
	       break;
	      }
	      else
	       AliasListItemLocal = AliasListItemLocal->next;
	     }
	    }
           }
           else{
            free(AliasListReplace->replacement_string);
            AliasListReplace->replacement_string = (char*)malloc(mem_length*sizeof(char));
            for(i=2,k=0; command_parse[i]; i++){
             j=0;
             while(command_parse[i][j]){
              AliasListReplace->replacement_string[k] = command_parse[i][j];
              k++;
              j++;
             }
             AliasListReplace->replacement_string[k] = (i==(command_count-1))?'\0':' ';
             k++;
            }
           }
          }
         }
        }
        else if(strcmp(command_parse[0],"unalias") == 0){
         if(command_count != 2){
         //Screw saying unalias:Incorrect number of arguements 
          write(STDERR_FILENO,"unalias: Incorrect number of arguments.\n",strlen("unalias: Incorrect number of arguments.\n"));
         }else{
          if(AliasList != NULL){
           AliasListIterator = AliasList;
           struct AliasListItem* TempItem=NULL;
           i=0;
           do{
            if(strcmp(AliasListIterator->alias_name,command_parse[1])==0){
             if(i==1) TempItem->next = AliasListIterator->next;
             if(i==0 && AliasListIterator->next==NULL) AliasList = NULL;
             if(i==0 && AliasListIterator->next!=NULL) AliasList = AliasList->next;
             free(AliasListIterator->alias_name);
             free(AliasListIterator->replacement_string);
             free(AliasListIterator);
	     break;
            }else{
	     TempItem = AliasListIterator;
             AliasListIterator = AliasListIterator->next;
            }
            i=1;
           }while(AliasListIterator);
          }
         }
        }
        else{ 
         // Create a new process fork() and init w/ execv() 
         int rc;
	 char* pr=NULL;
         rc = fork();
         if(rc == 0){	  
          FILE* redir_fp=NULL;
	  int file_error = 0;
	  if(redirection == 1){
	   fclose(stdout);
           redir_fp = fopen(redir_file,"w");
	   if (redir_fp==NULL){
            write(STDERR_FILENO,"Cannot write to file ",strlen("Cannot write to file "));
            write(STDERR_FILENO,redir_file,strlen(redir_file)); 
            write(STDERR_FILENO,".\n",strlen(".\n"));
	    file_error = 1;
	   }
	  }
	  if(file_error == 0){
           execv(line_arg,command_parse);
	   pr = (char*)malloc((strlen(line_arg)+100)*sizeof(char));
	   strcpy(pr,line_arg);
	   pr = strcat(pr,": Command not found.\n");
	   write(STDERR_FILENO,pr,strlen(pr));
	   //fclose(redir_fp);
	   free(pr);
	  }
	  free(line_arg);
	  free(line);
	  for(i=0;command_parse[i];i++)
	   free(command_parse[i]);
	  free(command_parse[i]);
	  free(command_parse);
	  if(interactive==0) fclose(batch_fp);
	  _exit(1);
         }

         else if(rc > 0){ // parent process
           wait(NULL); // wait for the child process to run
         }
	}
     
       }
       free(line_arg);
       if(alias_comm == 1){
        free(command_parse);
        free(linedup);
       }
       else{
        for(i=0; command_parse[i] ; i++)
	 free(command_parse[i]);
	free(command_parse[i]);
	free(command_parse);
       }
       }
       if(interactive)
        write(STDOUT_FILENO, prompt, prompt_len);  // Write the prompt "mysh> " to the user
      }
 //   }
    if(interactive == 0)
     fclose(batch_fp);
    if(nread == -1)
     free(line);
    int flag = 0;
    AliasListIterator = AliasList;
    while(AliasListIterator){
     free(AliasListIterator->alias_name);
     free(AliasListIterator->replacement_string);
     if(DelItem != NULL)
      free(DelItem);
     DelItem = AliasListIterator;
     AliasListIterator = AliasListIterator->next;
     flag = 1;
     }
     if(flag) free(DelItem);

  //}
 
  return 0;
}
