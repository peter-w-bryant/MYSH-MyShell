# Command Line Interpreter (Shell) Built on Top of Unix
## General
When a user types in a command, my shell creates a child process that executes the command the user entered and then prompts for more user input when it has finished.  At a high level, my shell is a simple loop that waits for input and fork()s a new child process to execute the command; the child process then exec()s the specified command while the parent process wait()s for the child to finish before continuing with the next iteration of the loop.

Besides the most basic function of executing commands, my shell (called mysh) provides the following three features: interactive vs. batch mode, output redirection, and aliasing.

## Features
### 2.1) Modes: Interactive vs. Batch
My shell can be run in two modes: interactive and batch, which is determined when the shell is started. If my shell is started with no arguments (i.e., ./mysh) , it will run in interactive mode; if my shell is given the name of a file (e.g., ./mysh batch-file), it runs in batch mode. 

In <b>interactive mode</b>, my shell displays the prompt "mysh>" and the user of the shell will type in a command at the prompt.

In <b>batch mode</b>, my shell is started by specifying a batch file on its command line; the batch file contains the list of commands (each on its own line) that should be executed. In batch mode, the shell echos each line read from the batch file back to the user (stdout) before executing it.

In <b>both interactive and batch mode</b>, my shell terminates when it sees the exit command on a line or reaches the end of the input stream (i.e., the end of the batch file or the user types 'Ctrl-D').  
### 2.2) Redirection
To enable a shell user who prefers to send the output of a program to a file rather than to the screen, I have included redirection (i.e. usually, a shell redirects standout output to a file with the '>' character; my shell includes this feature).

For example, if a user types "/bin/ls -la /tmp > output" into my shell, nothing is printed on the screen. Instead, the standard output of the ls program is rerouted to the file output. Additionally, if the output file exists before the shell is run, my shell overwrites it (after truncating it, which sets the file's size to zero bytes). 

The exact format of redirection is: 

<b>a command</b> (along with its arguments, if present), any number of white spaces (including none), the <b>redirection symbol ></b>, any number of white spaces (including none), followed by <b>a filename</b>.

As for a few special cases I have implemented:
<ul>
  <li>Multiple redirection operators (e.g. /bin/ls > > file.txt ), starting with a redirection sign (e.g. > file.txt).</li>
  <li>Multiple files to the right of the redirection sign (e.g. /bin/ls > file1.txt file2.txt).</li>
  <li>Not specifying an output file (e.g. /bin/ls > )</li>
</ul>
.. are all errors.  My shell prints: "Redirection misformatted". If the output file cannot be opened for some reason (e.g., the user doesn't have write permission or the name is an existing directory), my shell prints "Cannot write to file foo.txt." In these cases, my shell doesn't execute the command and continues to the next line.

### 2.3) Aliases
Many shells also contain functionality for aliases.   To see the aliases that are currently active in your Linux shell, you can type alias.    Basically, an alias is just a short-cut so that the user can type in something simple and have something more complex (or more safe) be executed.  
For example, you could set up:
mysh> alias ls /bin/ls
so that within this shell session, the user can simply type ls and the executable /bin/ls will be run.
Note that alias is an example of a "built-in" command. A built-in command means that the shell interprets this command directly; the shell does not exec() the built-in command and run it as a separate process; instead, the built-in command impacts how the shell itself runs.  
There are three ways that alias can be invoked in your shell. 
