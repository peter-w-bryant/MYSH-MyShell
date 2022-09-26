#include <string.h>   // strtok()
#include <sys/wait.h> // wait()
#include <unistd.h>   // fork(), execv(), _exit()
#include <stdlib.h>   // exit()
#include <ctype.h>    // isspace()
// #include <sys/types.h> // for locks, semaphores, and other types of IPC (interprocess communication)
#include <stdio.h> // printf() -> unused, now using write() instead

// Linked list node containing alias and corresponding executable path
struct AliasListItem
{
        char *alias_name;           // alias name
        char *replacement_string;   // the executable path we want to replace the alias with
        struct AliasListItem *next; // pointer to next node in linked list
};

int main(int argc, char *argv[])
{
        char const *exit_str = "exit";                  // String to compare against to exit
        char const *prompt = "mysh> ";                  // Prompt string
        int const prompt_len = 6;                       // Length of prompt string
        int in_interactive;                             // Flag to determine if we are in in_interactive mode
        size_t nread;                                   // # of bytes read by getline()
        char *line = NULL;                              // Pointer to MEM where a line will be read to
        FILE *batch_fp = NULL;                          // File pointer to batch file
        struct AliasListItem *AliasList = NULL;         // Pointer to the head of the linked list
        struct AliasListItem *AliasListIterator = NULL; // Pointer to the current node in the linked list
        struct AliasListItem *ToDeleteItem = NULL;      // Pointer to the node to be deleted in the linked list

        // We should have at max 2 arguments, the program name and the optional batch-file name
        if (argc > 2)
        {
                write(STDERR_FILENO, "Usage: mysh [batch-file]\n", strlen("Usage: mysh [batch-file]\n")); // Print error message
                exit(1);                                                                                  // Exit with error
        }

        in_interactive = (argc == 1) ? 1 : 0;                     // If we have 1 argument, we are in in_interactive mode
        batch_fp = (in_interactive) ? NULL : fopen(argv[1], "r"); // If we are in batch mode, open the batch file

        // If we are in batch mode and the batch file could not be opened
        if (!in_interactive && batch_fp == NULL)
        {
                write(STDERR_FILENO, "Error: Cannot open file ", strlen("Error: Cannot open file ")); // Print error part 1
                write(STDERR_FILENO, argv[1], strlen(argv[1]));                                       // Print the file name
                write(STDERR_FILENO, ".\n", strlen(".\n"));                                           // Print error part 2
                exit(1);                                                                              // Exit with error
        }

        // If we are in in_interactive mode, print the prompt
        if (in_interactive)
                write(STDOUT_FILENO, prompt, prompt_len); // Print the prompt

        // While we have have not reached the end of the batch file or we are in in_interactive mode
        while ((nread = getline(&line, &nread, in_interactive ? stdin : batch_fp)) != -1) // Read a line from the batch file or stdin
        {
                char const *delimiter = " "; // Delimiter for strtok()
                char *token;                 // Pointer to the current token
                char *linedup;               // Pointer to a copy of the line
                char *linedup_bkp;           // Pointer to a copy of the line, bkp -> backup
                char *line_arg;              // Pointer to the line argument
                char **command_parse;        // Pointer to the command parse
                int line_str_count = 0;      // Counters
                int contains_char = 0;       // Flag to determine if the line contains any non-whitespace characters
                int alias_comm = 0;          // Flag to determine if we are in alias command

                // If we are in batch mode, echo the command back to the user
                if (in_interactive == 0)
                        write(STDOUT_FILENO, line, strlen(line));

                line[strlen(line) - 1] = '\0';            // Replace the newline character with a null terminator
                write(STDOUT_FILENO, line, strlen(line)); // Print the line
                write(STDOUT_FILENO, "\n", strlen("\n")); // Print a new line

                // Check if the line contains any non-whitespace characters
                int y = 0; // Counter
                while (line[y])
                {
                        // If the current character is not a whitespace character
                        if (!isspace(line[y]))
                                contains_char = 1; // Set the flag to 1
                        y++;                       // Increment the character counter
                }

                // If contains_char is 1, then the line contains non-whitespace characters, so I will parse it
                if (contains_char == 1)
                {
                        // Flags
                        int isspace_before = 0; // 1 if we have a whitespace character before the current character and not after
                        int isspace_after = 0;  // 1 if we have a whitespace character after the current character but not before
                        int isspace_both = 0;   // 1 if we have a whitespace character before and after the current character
                        int isspace_none = 0;   // 1 if we have no whitespace characters before and after the current character

                        int redir_location; // String index (of the current line) of the redirection character '>' in the line
                        int i = 0;          // Character counter

                        // Iterate through the line, finding the number of tokens and the location of the redirection character
                        while (line[i])
                        {
                                // If the first character is not a whitespace character
                                if (i == 0 && !isspace(line[i]))
                                        line_str_count++; // There is atleast one command
                                // If the current character is a whitespace character and the next character is not a white space character
                                else if (isspace(line[i]) && !isspace(line[i + 1]) && line[i + 1] != '\0')
                                        line_str_count++; // There must be another command

                                // If the current character is not the first character
                                if (i > 0)
                                {
                                        // -- Redirection character cases --
                                        // If the current character is a redirection character, with whitespace before and not after
                                        if (line[i] == '>' && isspace(line[i - 1]) && !isspace(line[i + 1]))
                                        {
                                                isspace_before = 1;
                                                redir_location = line_str_count - 1; // No space after the redirection character implies that the last
                                                                                     // string is the redirection character
                                        }
                                        // If the current character is a redirection character, with whitespace before and after
                                        if (line[i] == '>' && isspace(line[i - 1]) && isspace(line[i + 1]))
                                        {
                                                isspace_both = 1;
                                                redir_location = line_str_count - 2; // Space after the redirection character implies that the second
                                                                                     // to last string is the redirection character
                                        }
                                        // If the current character is a redirection character, with no whitespace before but whitespace after
                                        if (line[i] == '>' && !isspace(line[i - 1]) && isspace(line[i + 1]))
                                        {
                                                isspace_after = 1;
                                                redir_location = line_str_count - 1; // No space before the redirection character implies that the last
                                                                                     // string is the redirection character
                                        }
                                        // If the current character is a redirection character, with no whitespace before and not after either
                                        if (line[i] == '>' && !isspace(line[i + 1]) && !isspace(line[i - 1]))
                                        {
                                                isspace_none = 1;
                                                redir_location = line_str_count - 1; // No space before or after the redirection character implies that the
                                                                                     // last string is the redirection character
                                        }
                                }

                                i++; // Increment the character counter
                        }

                        i = 0;                                                                  // Reset the character counter
                        command_parse = (char **)malloc((line_str_count + 1) * sizeof(char *)); // Allocate memory for the parsed commands and null terminator
                        int redir_char_counter = 0;                                             // Create a new character counter
                        char redir_file[512];                                                   // Buffer for the redirection file name, 512-1 is the max file name length

                        // Flags
                        int exec_program = 1;     // 1 if we are executing a program, 0 if we find an error
                        int contains_command = 0; // 1 if the line contains a command, 0 if no commands are given
                        int redirection = 0;      // 1 if we are redirecting the output of a program, 0 if we are not
                        int file = 0;             // 1 if there is a file to redirect the output to, 0 if there is not
                        int file_done = 0;        // 1 if we have finished reading the file name, 0 if we have not

                        // Iterate through the line, parsing the commands
                        while (line[i])
                        {
                                if (!isspace(line[i]) && line[i] != '>') // If the current character is not a whitespace character, and not the redirection character,
                                        contains_command = 1;            // we have a command to execute.

                                if (redirection == 1 && line[i] == '>') // If we find more than one redirection character,
                                        exec_program = 0;               // we have an error and we will not execute the program.

                                if (redirection && !isspace(line[i])) // If we are redirecting the output of a program, and we find a non-whitespace character,
                                        file = 1;                     // we have a file name to read.

                                if (line[i] == '>')      // If we find a redirection character,
                                        redirection = 1; // we are redirecting the output of a program.

                                if (file == 1 && (isspace(line[i]) || line[i] == '\0')) // If we are reading a file name, and we find a whitespace character or the end of the line,
                                        file_done = 1;                                  // we have finished reading the file name.

                                if (contains_command == 0 && redirection == 1) // If we are not given a command to execcute, and we are redirecting the output of a program,
                                        exec_program = 0;                      // we have an error and we will not execute the program.

                                if (file_done && !isspace(line[i])) // If we have finished reading the file name, and we find a non-whitespace character,
                                        exec_program = 0;           // we have an error and we will not execute the program.

                                if (file && !file_done) // If we are reading a file name, and we have not finished reading the file name,
                                {
                                        redir_file[redir_char_counter] = line[i]; // Store the current character in the file name buffer
                                        redir_char_counter++;                     // Increment the charcter counter for the file name
                                }

                                i++;
                        }
                        redir_file[redir_char_counter] = '\0'; // Add the null terminator to the file name buffer

                        if (redirection && file == 0) // If we are redirecting the output of a program, and we are not given a file name,
                                exec_program = 0;     // we have an error and we will not execute the program.

                        if (exec_program == 0) // If we have an error, print an error message and continue execution.
                                write(STDERR_FILENO, "Redirection misformatted.\n", strlen("Redirection misformatted.\n"));

                        // Zero out/Initialize character counters
                        i = 0;                // Line character counter
                        int j = 0, k = 0;     // Command character counters
                        int num_commands = 0; // Number of commands

                        // Iterate through the line, parsing the commands
                        while (line[i])
                        {
                                // If the first character is not a white space character
                                if (i == 0 && !isspace(line[i]))
                                {
                                        j = i; // Zero out the command character counter

                                        // Iterate through the line, and count the number of characters in the first command
                                        while (!isspace(line[j]) && line[j] != '\0')
                                                j++;

                                        command_parse[num_commands] = (char *)malloc((j + 1) * sizeof(char)); // Allocate memory for the first command
                                        j = i;                                                                // Zero out the command character counter
                                        // Iterate through the line, and copy the first command into the command_parse array
                                        while (!isspace(line[j]) && line[j] != '\0')
                                        {
                                                command_parse[num_commands][j] = line[j]; // Copy the current character into the command_parse array
                                                j++;                                      // Increment the command character counter
                                        }
                                        command_parse[num_commands][j] = '\0'; // Add the null terminator to the command_parse array for the first command
                                        num_commands++;                        // Increment the number of commands counter
                                        i = j;                                 // Set the line character counter to the command character counter, so we can continue parsing the line
                                                                               // from the end of the first command.
                                }

                                // If the current character is a whitespace character, and the next character is not a whitespace character or the end of the line
                                else if (isspace(line[i]) && !isspace(line[i + 1]) && line[i + 1] != '\0')
                                {
                                        j = i + 1; // Set the command character counter to the next character
                                        k = 0;     // Zero out the command character counter

                                        // Iterate through the line, and count the number of characters in the next command
                                        while (!isspace(line[j]) && line[j] != '\0')
                                        {
                                                k++; // Increment the number of characters in the next command
                                                j++; // Increment the index of the next character
                                        }
                                        command_parse[num_commands] = (char *)malloc((k + 1) * sizeof(char)); // Allocate memory for the next command
                                        j = i + 1;                                                            // Set the command index back to the start of this command
                                        k = 0;                                                                // Zero out the command character counter

                                        // Iterate through the line, and copy the next command into the command_parse array
                                        while (!isspace(line[j]) && line[j] != '\0')
                                        {
                                                command_parse[num_commands][k] = line[j]; // Copy the current character into the command_parse array
                                                j++;                                      // Increment the index of the next character
                                                k++;                                      // Increment the command character counter
                                        }
                                        command_parse[num_commands][k] = '\0'; // Add the null terminator to the command_parse array for the next command
                                        num_commands++;                        // Increment the number of commands counter
                                        i = j;                                 // Set the line character counter to the command character counter, so we can continue parsing the line
                                                                               // from the end of the next command.
                                }
                                // If the current character is a whitespace character, increment the line character counter
                                else
                                        i++;
                        }
                        command_parse[num_commands] = NULL; // Add the NULL terminator to the command_parse array, so execv() knows when to stop

                        // If we are going forward with executing the program, and we are redirecting its output
                        if (redirection && exec_program)
                        {
                                // If there is whitespace before the redirection character but not after it, free the memory after the file name
                                if (isspace_before)
                                {
                                        // Iterate through the command_parse array, freeing the memory after the file name, and stripping the command_parse array of all characters
                                        // starting with the redirection character and after.
                                        for (i = redir_location + 1; command_parse[i]; i++)
                                                free(command_parse[i]);
                                        free(command_parse[i]);               // Free the memory for the NULL terminator
                                        free(command_parse[redir_location]);  // Free the memory for the redirection character
                                        command_parse[redir_location] = NULL; // Set the index of the redirection character to NULL
                                }

                                // If there is whitespace after the redirection character but not before it, remove the redirection character
                                if (isspace_after)
                                {
                                        command_parse[redir_location][strlen(command_parse[redir_location]) - 1] = '\0';
                                }

                                // If there is no whitespace before or after the redirection character, remove the redirection character and everything after it
                                if (isspace_none)
                                {
                                        for (i = 0; command_parse[redir_location][i]; i++)
                                        {
                                                if (command_parse[redir_location][i] == '>')
                                                {
                                                        command_parse[redir_location][i] = '\0';
                                                        break;
                                                }
                                        }
                                        // Free the memory for the redirection character and everything after it
                                        for (i = redir_location + 1; command_parse[i]; i++)
                                                free(command_parse[i]);
                                        free(command_parse[i]);                   // Free the memory for the NULL terminator
                                        command_parse[redir_location + 1] = NULL; // Set a new NULL terminator
                                }

                                // If either of the both or after flags is true, we need to free the memory for at and after the redirection character
                                if (isspace_both || isspace_after)
                                {
                                        // Iterate through the command_parse array, freeing the memory at and after the redirection character
                                        for (i = redir_location + 2; command_parse[i]; i++)
                                                free(command_parse[i]);
                                        free(command_parse[i]);
                                        free(command_parse[redir_location + 1]);
                                        command_parse[redir_location + 1] = NULL;
                                }
                        }

                        // Zero out counters
                        i = 0;
                        int is_alias = 0; // Flag to indicate if the command is an alias, 1 if it is, 0 if it is not

                        AliasListIterator = AliasList; // Set the AliasListIterator to the start of the AliasList

                        // Iterate through the AliasList, and check if the first command is an alias
                        while (AliasListIterator)
                        {
                                // If the first command is an alias
                                if (strcmp(AliasListIterator->alias_name, command_parse[0]) == 0)
                                {
                                        is_alias = 1;                                                                                     // Set flag to indicate that the command is an alias
                                        linedup = (char *)malloc((strlen(AliasListIterator->replacement_string) + 1) * sizeof(char));     // Allocate memory for the alias replacement string
                                        linedup_bkp = (char *)malloc((strlen(AliasListIterator->replacement_string) + 1) * sizeof(char)); // Allocate memory for the alias replacement string backup
                                        strcpy(linedup, AliasListIterator->replacement_string);                                           // Copy the alias replacement string into linedup
                                        strcpy(linedup_bkp, AliasListIterator->replacement_string);                                       // Copy the alias replacement string into linedup_bkp
                                }
                                AliasListIterator = AliasListIterator->next; // Set the AliasListIterator to the next node in the AliasList
                        }

                        // If the command is an alias
                        if (is_alias == 1)
                        {
                                token = strtok(linedup_bkp, delimiter); // Tokenize the alias replacement string
                                line_str_count = 0;                     // Zero out the line string counter

                                // Iterate through the alias replacement string, and count the number of strings
                                while (token != NULL)
                                {
                                        token = strtok(NULL, delimiter); // Man Pages: On the first call to strtok(), the string to be parsed should be specified in str (first arg),
                                                                         // in each subsequent call that should parse the same string, str must be NULL.
                                        line_str_count++;                // Increment the line string counter
                                }

                                // Free the memory for the command_parse array
                                for (i = 0; command_parse[i]; i++)
                                        free(command_parse[i]); // Free the memory for the command_parse array
                                free(command_parse[i]);         // Free the memory for the NULL terminator
                                free(command_parse);            // Free the memory for the pointer to the command_parse array
                                free(linedup_bkp);              // Free the memory for the alias replacement string backup

                                command_parse = (char **)malloc((line_str_count + 1) * sizeof(char *)); // Allocate memory for the command_parse array
                                token = strtok(linedup, delimiter);                                     // Tokenize the alias replacement string

                                i = 0; // Zero out the counter

                                // Iterate through the alias replacement string, and copy the strings into the command_parse array
                                while (token != NULL)
                                {
                                        command_parse[i] = token;
                                        i++;
                                        token = strtok(NULL, delimiter);
                                }
                                command_parse[i] = NULL; // Set the last index of the command_parse array to NULL
                                alias_comm = 1;          // Set the alias_comm flag to 1 to indicate that the command is an alias
                        }

                        line_arg = (char *)malloc((strlen(command_parse[0]) + 1) * sizeof(char)); // Allocate memory for the line_arg string, which will be used to store the command
                        strcpy(line_arg, command_parse[0]);                                       // Copy the command into the line_arg string

                        // If the command is the exit command, free all memory and exit the shell
                        if (strcmp(line, exit_str) == 0)
                        {
                                free(line_arg); // Free the memory for the line_arg string
                                for (i = 0; command_parse[i]; i++)
                                        free(command_parse[i]); // Free the memory for the command_parse array
                                free(command_parse[i]);         // Free the memory for the NULL terminator
                                free(command_parse);            // Free the memory for the pointer to the command_parse array
                                free(line);                     // Free the memory for the line string
                                if (in_interactive == 0)        // If the shell is in batch mode,
                                        fclose(batch_fp);       // Close the batch file
                                int alias_delete_flag = 0;      // Flag to indicate if the alias should be deleted, 1 if it should be deleted, 0 if it should not be deleted
                                AliasListIterator = AliasList;  // Set the AliasListIterator to the start of the AliasList

                                // Iterate through the AliasList, and repeatedly free the memory for the AliasList
                                while (AliasListIterator)
                                {
                                        free(AliasListIterator->alias_name);         // Free the memory for the alias name
                                        free(AliasListIterator->replacement_string); // Free the memory for the alias replacement string
                                        // Free the memory for the AliasListIterator
                                        if (ToDeleteItem != NULL)
                                                free(ToDeleteItem);
                                        ToDeleteItem = AliasListIterator;            // Set the ToDeleteItem to the AliasListIterator
                                        AliasListIterator = AliasListIterator->next; // Set the AliasListIterator to the next node in the AliasList
                                        alias_delete_flag = 1;                       // Set the flag to 1 to indicate that the AliasList is not empty
                                }
                                if (alias_delete_flag)
                                        free(ToDeleteItem); // Free the last node in the AliasList
                                exit(0);                    // Exit the shell, no errors
                        }

                        // Execute the command if exec_program flag is set to 1
                        if (exec_program)
                        {
                                // If the command is the alias command, the alias is a built-in command, so it is handled here
                                if (strcmp(command_parse[0], "alias") == 0)
                                {
                                        // If the alias command has no arguments, print the current aliases
                                        if (line_str_count == 1)
                                        {
                                                AliasListIterator = AliasList;
                                                // Iterate through the AliasList, and print the aliases
                                                while (AliasListIterator)
                                                {
                                                        write(STDOUT_FILENO, AliasListIterator->alias_name, strlen(AliasListIterator->alias_name) * sizeof(char));
                                                        write(STDOUT_FILENO, " ", sizeof(char));
                                                        write(STDOUT_FILENO, AliasListIterator->replacement_string, strlen(AliasListIterator->replacement_string) * sizeof(char));
                                                        write(STDOUT_FILENO, "\n", sizeof(char));
                                                        AliasListIterator = AliasListIterator->next;
                                                }
                                        }
                                        // If the alias command has one additional argument that matches a current alias name, print the alias name and the replacement string
                                        else if (line_str_count == 2)
                                        {
                                                AliasListIterator = AliasList;
                                                // Iterate through the AliasList, and print the alias name and the replacement string if the alias name matches the argument
                                                while (AliasListIterator)
                                                {
                                                        if (strcmp(AliasListIterator->alias_name, command_parse[1]) == 0)
                                                        {
                                                                write(STDOUT_FILENO, AliasListIterator->alias_name, strlen(AliasListIterator->alias_name) * sizeof(char));
                                                                write(STDOUT_FILENO, " ", sizeof(char));
                                                                write(STDOUT_FILENO, AliasListIterator->replacement_string, strlen(AliasListIterator->replacement_string) * sizeof(char));
                                                                write(STDOUT_FILENO, "\n", sizeof(char));
                                                        }
                                                        AliasListIterator = AliasListIterator->next;
                                                }
                                        }
                                        // If the alias command has two or more additional arguments, we must create a new alias, but we check for valid alias names first
                                        else if (line_str_count >= 3)
                                        {
                                                // If the alias name is 'alias', 'unalias', or 'exit', print an error message
                                                if (strcmp(command_parse[1], "alias") == 0 || strcmp(command_parse[1], "unalias") == 0 || strcmp(command_parse[1], "exit") == 0)
                                                {
                                                        write(STDERR_FILENO, "alias: Too dangerous to alias that.\n", strlen("alias: Too dangerous to alias that.\n"));
                                                }
                                                // If the alias name is not those which are invalid, create the alias
                                                else
                                                {
                                                        struct AliasListItem *AliasListReplace = NULL; // A pointer to an AliasListItem struct to be used to replace an existing alias

                                                        // Find the last node in the AliasList (2 cases: empty list, or non-empty list)
                                                        // 1. If the AliasList is empty, set the AliasListIterator to NULL
                                                        if (AliasList == NULL)
                                                                AliasListIterator = AliasList;
                                                        // 2. If the AliasList is not empty, set the AliasListIterator to the last node in the AliasList
                                                        else
                                                        {
                                                                AliasListIterator = AliasList;
                                                                while (AliasListIterator)
                                                                {
                                                                        // If the alias name already exists, set the AliasListReplace to the node that contains the alias name
                                                                        if (strcmp(AliasListIterator->alias_name, command_parse[1]) == 0)
                                                                                AliasListReplace = AliasListIterator;
                                                                        AliasListIterator = AliasListIterator->next;
                                                                }
                                                        }
                                                        // If the alias name does not already exist, create a new node in the AliasList
                                                        if (AliasListReplace == NULL)
                                                        {
                                                                AliasListIterator = (struct AliasListItem *)malloc(sizeof(struct AliasListItem));              // Allocate memory for the new node
                                                                AliasListIterator->next = NULL;                                                                // Set the next pointer to NULL
                                                                AliasListIterator->alias_name = (char *)malloc((strlen(command_parse[1]) + 1) * sizeof(char)); // Allocate memory for the alias name
                                                                strcpy(AliasListIterator->alias_name, command_parse[1]);                                       // Copy the alias name into the new node
                                                        }
                                                        int mem_length = 0; // The length of the memory to allocate for the replacement string
                                                        for (i = 2; command_parse[i]; i++)
                                                                mem_length += strlen(command_parse[i]); // Add the length of each argument to the replacement string
                                                        mem_length += line_str_count - 2 + 1;           // For each argument, there is a space between the arguments and the null terminator

                                                        // If the alias name already exists, replace the replacement string
                                                        if (AliasListReplace == NULL)
                                                        {
                                                                AliasListIterator->replacement_string = (char *)malloc(mem_length * sizeof(char)); // Allocate memory for the replacement string
                                                                // For each argument after 'alias', copy the argument into the replacement string
                                                                for (i = 2, k = 0; command_parse[i]; i++)
                                                                {
                                                                        j = 0;
                                                                        while (command_parse[i][j])
                                                                        {
                                                                                AliasListIterator->replacement_string[k] = command_parse[i][j]; // Copy each character from the argument into the replacement string
                                                                                k++;                                                            // Increment the index of the replacement string
                                                                                j++;                                                            // Increment the index of the argument
                                                                        }
                                                                        AliasListIterator->replacement_string[k] = (i == (line_str_count - 1)) ? '\0' : ' '; // If the argument is the last argument,
                                                                                                                                                             // set the replacement string to null terminator,
                                                                                                                                                             // otherwise set the replacement string to a space.
                                                                        k++;                                                                                 // Increment the index of the replacement string
                                                                }
                                                                struct AliasListItem *AliasListItemLocal = NULL;
                                                                // If the AliasList is empty, set the AliasList to the new node
                                                                if (AliasList == NULL)
                                                                        AliasList = AliasListIterator; // Set the AliasList to the new node
                                                                // If the AliasList is not empty, set the last node in the AliasList to the new node
                                                                else
                                                                {
                                                                        AliasListItemLocal = AliasList;
                                                                        // Iterate through the AliasList until the last node is reached
                                                                        while (AliasListItemLocal)
                                                                        {
                                                                                // If the next node is NULL, we are at the last node, so set the next node to the new node
                                                                                if (AliasListItemLocal->next == NULL)
                                                                                {
                                                                                        AliasListItemLocal->next = AliasListIterator;
                                                                                        break;
                                                                                }
                                                                                else
                                                                                        AliasListItemLocal = AliasListItemLocal->next; // Otherwise, set the current node to the next node to continue iterating
                                                                        }
                                                                }
                                                        }
                                                        // If the alias name already exists, replace the replacement string
                                                        else
                                                        {
                                                                free(AliasListReplace->replacement_string);                                       // Free the memory allocated for the current replacement string
                                                                AliasListReplace->replacement_string = (char *)malloc(mem_length * sizeof(char)); // Allocate memory for the new replacement string
                                                                // For each argument after 'alias', copy the argument into the replacement string
                                                                for (i = 2, k = 0; command_parse[i]; i++)
                                                                {
                                                                        j = 0;
                                                                        // Copy each character from the argument into the replacement string
                                                                        while (command_parse[i][j])
                                                                        {
                                                                                AliasListReplace->replacement_string[k] = command_parse[i][j];
                                                                                k++; // Increment the index of the replacement string
                                                                                j++; // Increment the index of the argument
                                                                        }
                                                                        AliasListReplace->replacement_string[k] = (i == (line_str_count - 1)) ? '\0' : ' '; // If the argument is the last argument,
                                                                                                                                                            // set the replacement string to null terminator,
                                                                                                                                                            // otherwise set the replacement string to a space.
                                                                        k++;                                                                                // Increment the index of the replacement string
                                                                }
                                                        }
                                                }
                                        }
                                }
                                // If the command is the unalias command, this is another built-in command, so it is handled here
                                else if (strcmp(command_parse[0], "unalias") == 0)
                                {
                                        // There should be exactly one argument after the unalias command, so if there is not exactly one argument, print an error message
                                        if (line_str_count != 2)
                                                write(STDERR_FILENO, "unalias: Incorrect number of arguments.\n", strlen("unalias: Incorrect number of arguments.\n")); // Print an error message
                                        // If there is exactly one argument after the unalias command, continue execution
                                        else
                                        {
                                                // If the AliasList is non empty, continue execution
                                                if (AliasList != NULL)
                                                {
                                                        AliasListIterator = AliasList;         // Set the iterator to the first node in the AliasList
                                                        struct AliasListItem *TempItem = NULL; // A temporary node to store the previous node
                                                        int is_first = 1;                      // A flag to indicate if the current node is the first node in the AliasList, 0 if it is not, 1 if it is
                                                        // Iterate through the AliasList until the alias name is found or the end of the AliasList is reached
                                                        do
                                                        {
                                                                // If the current node is the node to be deleted
                                                                if (strcmp(AliasListIterator->alias_name, command_parse[1]) == 0)
                                                                {
                                                                        // If the current node is not the first node in the AliasList
                                                                        if (is_first == 0)
                                                                                TempItem->next = AliasListIterator->next; // Set the previous node's next pointer to the next node

                                                                        // If the current node is the first node in the AliasList, and the current node is the last node in the AliasList
                                                                        if (is_first == 1 && AliasListIterator->next == NULL)
                                                                                AliasList = NULL; // Set the AliasList to NULL

                                                                        // If the current node is the first node in the AliasList, and the current node is not the last node in the AliasList
                                                                        if (is_first == 1 && AliasListIterator->next != NULL)
                                                                                AliasList = AliasList->next; // Set the first node in the AliasList to the next node

                                                                        free(AliasListIterator->alias_name);         // Free the memory allocated for the alias name
                                                                        free(AliasListIterator->replacement_string); // Free the memory allocated for the replacement string
                                                                        free(AliasListIterator);                     // Free the pointer to the node
                                                                        break;
                                                                }
                                                                // If the current node is not the node to be deleted
                                                                else
                                                                {
                                                                        TempItem = AliasListIterator;                // Set the temporary node to the current node
                                                                        AliasListIterator = AliasListIterator->next; // Set the current node to the next node
                                                                }
                                                                is_first = 0; // Set the flag to 0 to indicate that the current node is not the first node
                                                        } while (AliasListIterator);
                                                }
                                        }
                                }
                                // If the command is not alias or unalias, we need to executate a command that is not built in
                                else
                                {
                                        // Create a new process fork() and use execv() to execute the command
                                        int rc;          // The return code of the fork() function
                                        char *pr = NULL; // A pointer to the path of the command
                                        rc = fork();     // Create a new process

                                        // If the fork() function is successful, continue execution
                                        if (rc == 0)
                                        {
                                                FILE *redir_fp = NULL; // A file pointer to the file to be redirected to
                                                int file_error = 0;    // A flag to indicate if there was an error opening the file, 0 if there was no error, 1 if there was an error

                                                // If the command is to be redirected to a file
                                                if (redirection == 1)
                                                {
                                                        fclose(stdout);                    // Close the standard output
                                                        redir_fp = fopen(redir_file, "w"); // Open the file to be redirected to

                                                        // If we could not open the file, set the file_error flag to 1 and print an error message
                                                        if (redir_fp == NULL)
                                                        {
                                                                write(STDERR_FILENO, "Cannot write to file ", strlen("Cannot write to file "));
                                                                write(STDERR_FILENO, redir_file, strlen(redir_file));
                                                                write(STDERR_FILENO, ".\n", strlen(".\n"));
                                                                file_error = 1; // Set the file_error flag to 1
                                                        }
                                                }

                                                // If there was no error opening the file, continue execution
                                                if (file_error == 0)
                                                {
                                                        execv(line_arg, command_parse); // Execute the command

                                                        // If the execv() function returns, there was an error executing the command, so print an error message
                                                        pr = (char *)malloc((strlen(line_arg) + 100) * sizeof(char)); // Allocate memory for the path of the command, and a 100 byte buffer for the error message
                                                        strcpy(pr, line_arg);                                         // Copy the command to the path of the command
                                                        pr = strcat(pr, ": Command not found.\n");                    // Append the error message to the path of the command
                                                        write(STDERR_FILENO, pr, strlen(pr));                         // Print the error message
                                                        free(pr);                                                     // Free the memory allocated for the path of the command
                                                }
                                                free(line_arg); // Free the memory allocated for the command
                                                free(line);     // Free the memory allocated for the line

                                                // Iterate through the command_parse array and free the memory allocated for each element
                                                for (i = 0; command_parse[i]; i++)
                                                        free(command_parse[i]);
                                                free(command_parse[i]); // Free the memory allocated for the last element in the command_parse array
                                                free(command_parse);    // Free the memory allocated for the pointer to the command_parse array

                                                // If in batch mode, close the file pointer to the batch file
                                                if (in_interactive == 0)
                                                        fclose(batch_fp);
                                                _exit(1); // Exit the child process
                                        }
                                        // If the fork() function is unsuccessful, just wait for the child process to finish
                                        else if (rc > 0)
                                        {
                                                wait(NULL); // wait for the child process to run
                                        }
                                }
                        }
                        // After the command has been executed, free the memory allocated for the line and the command_parse array
                        free(line_arg); // Free the memory allocated for the command
                        // If the command was an alias, free the pointer to the command_parse array and the memory allocated for linedup
                        if (alias_comm == 1)
                        {
                                free(command_parse);
                                free(linedup);
                        }
                        // If it was not an alias, free the memory allocated for each element in the command_parse array, and the memory allocated for the last element in the command_parse array
                        else
                        {
                                // Iterate through the command_parse array and free the memory allocated for each element
                                for (i = 0; command_parse[i]; i++)
                                        free(command_parse[i]);
                                free(command_parse[i]); // Free the memory allocated for the last element in the command_parse array
                                free(command_parse);    // Free the memory allocated for the pointer to the command_parse array
                        }
                }
                // If there were no non-whitespace characters in the line, write the prompt to STDOUT again
                if (in_interactive)
                        write(STDOUT_FILENO, prompt, prompt_len); // Write the prompt "mysh> " to the user
        }
        // After we have reached the end of the file or we have finished the user input, free the memory allocated
        // If we were in batch mode, close the file pointer to the batch file
        if (in_interactive == 0)
                fclose(batch_fp);
        // If nread = -1, then no bytes were read into the line buffer, so free the pointer to the line buffer
        if (nread == -1)
                free(line);
        int alias_delete_flag = 0; // If we still need to delete the last node in the alias list, set this flag to 1
        AliasListIterator = AliasList;

        // Iterate through the alias list and free the memory allocated for each node
        while (AliasListIterator)
        {
                free(AliasListIterator->alias_name);         // Free the memory allocated for the alias name
                free(AliasListIterator->replacement_string); // Free the memory allocated for the replacement string
                if (ToDeleteItem != NULL)
                        free(ToDeleteItem);                  // Free the memory allocated for the node to be deleted
                ToDeleteItem = AliasListIterator;            // Set the node to be deleted to the current node
                AliasListIterator = AliasListIterator->next; // Set the current node to the next node
                alias_delete_flag = 1;                       // Set the alias_delete_flag to 1
        }
        // If the alias_delete_flag is 1, free the memory allocated for the last node in the alias list
        if (alias_delete_flag)
                free(ToDeleteItem);
        return 0;
}
