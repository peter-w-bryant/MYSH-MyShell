# Command Line Interpreter (Shell) Built on Top of Unix
## What is a shell?
A command line interpreter or shell is blanket term for any program that allows a user to enter commands and then executes those commands to the operating system.<sup>[1](https://www.lifewire.com/what-is-a-command-line-interpreter-2625827)</sup>

You can find out what shell you are currently running by executing ```echo $SHELL``` at a prompt. You can read more about the functionality of the shell you are running by looking at its documentation (man pages), in my case I access them with ```man /bin/bash```. My project definitely does not implement as much functionality as most shells.

## General
When a user types in a command, my shell creates a child process that executes the command the user entered and then prompts for more user input when it has finished.  At a high level, my shell is a simple loop that waits for input and fork()s a new child process to execute the command; the child process then exec()s the specified command while the parent process wait()s for the child to finish before continuing with the next iteration of the loop.

<p align="center">
  <img src='https://github.com/peter-w-bryant/MYSH-MyShell/blob/main/images/flowOfExecution.png' width='500px' height='auto'><br>
  <i>Figure 1: High-Level Flow of Execution Diagram</i>
</p>

In essence, my shell: repeatedly prints a prompt (if in interactive mode), parses the input, executes the command specified on that line of input, and waits for the command to finish.

Besides the most basic function of executing commands, my shell (called mysh) provides the following three features: 
<ol>
  <li>interactive vs. batch mode</li>
  <li>output redirection</li>
  <li>aliasing</li>
</ol>

All of which are described in more detail below.

## Features
### Interactive vs. Batch Mode
My shell can be run in two modes: interactive and batch, which is determined when the shell is started. If my shell is started with no arguments (e.g. ```./mysh```) , it will run in interactive mode; if my shell is given the name of a file (e.g. ```./mysh batch-file```), it runs in batch mode. 

One aspect that is very important to note is that in <b>both interactive and batch mode</b>, my program uses ```execv()``` to execute the new command that is read in while executing in either mode. Since ```execv()``` does not search the PATH environment variable, my shell requires that full paths are specified for all commands. In order to identify the location of executables, Linux provides the very useful ```which``` command which will provide the absolute path to the executable. As a concrete example, suppose you would like to run the ```ls``` shell command to list files in the current working directory. If executing in my shell,

```mysh> ls```

will not work. The user would need to run, ```which ls``` from their command line, get the absolute path of the executable such as ```/bin/ls```, and then run the executable as,

```mysh> /bin/ls```

Additionally, in <b>both interactive and batch mode</b>, my shell terminates when it sees the user execute the ```exit``` command or it reaches the end of the input stream (i.e. the end of the batch file or the user types 'Ctrl-D').

#### Interactive Mode
After compiling the project, simply run the executable in order to enter <b>interactive mode</b>,

```./mysh```

In <b>interactive mode</b>, my shell displays the prompt "mysh>" and the user can type in a command at the prompt by specifying the absolute path of the executable (such as the example we had using ```ls``` above).

#### Batch Mode
In <b>batch mode</b>, my shell is started by specifying a batch file on the command line; the batch file contains the list of commands (each on its own line) that should be executed. In batch mode, the shell echos each line read from the batch file back to the user (stdout) before executing it. For example, if a text file named <i>input.txt</i> with the following contents was provided as a batch file,

```
/bin/echo Running interactively
exit
```

Then the user can pass the commands from <i>input.txt</i> using the command,

```./mysh input.txt``` and my program would first print out ```/bin/echo Running interactively``` then it would execute the command, and exit. The total output for this example would be,

```
/bin/echo Running interactively
Running interactively
exit
```

### Redirection
To enable a shell user who prefers to send the output of a program to a file rather than to stdout, I have included the functionality to handle redirection.

For example, if a user types ```/bin/ls -la /tmp > output.txt``` into my shell, nothing is printed on the screen. Instead, the standard output of the ls program is rerouted to the file output.txt. Additionally, if the output file exists before the shell is run, my shell overwrites it (after truncating it, setting the file's size to zero bytes). For this example, we are listing all files in the temporary ```/tmp``` directory in a long list format and writing it to the output.txt file.

The exact format of redirection is: 

``` /path-to-executable -options > file-location ```

<b>a path to the executable</b> (along with any optional arguments, if present), the <b>redirection symbol ></b>, followed by <b>a filename</b>. Additionally, my shell can handle any number of white spaces (including none), so our above example can be written in the following equivalent ways,

```
mysh> /bin/ls -la             /tmp       >     output.txt
mysh> /bin/ls -la /tmp>output.txt
```

There are, however, a few special cases (all of which are treated as errors):
<ul>
  <li>Multiple redirection operators (e.g. /bin/ls > > file.txt ), or starting with a redirection sign (e.g. > file.txt).</li>
  <li>Multiple files to the right of the redirection sign (e.g. /bin/ls > file1.txt file2.txt)</li> 
  <li>Not specifying an output file (e.g. /bin/ls > )</li>
</ul>

In any of these cases, my shell prints: <i>Redirection misformatted</i>. If the output file cannot be opened for some reason (e.g., the user doesn't have write permission or the name is an existing directory), my shell prints <i>Cannot write to file foo.txt.</i> In these cases, my shell doesn't execute the command and continues to the next line.

### Aliasing

Most shells have the functionality for aliases. In Linux, you can use the ```alias``` command to list all currently active aliases.<sup>[2](https://www.mediacollege.com/linux/command/alias.html#:~:text=To%20see%20a%20list%20of,type%20alias%20at%20the%20prompt.&text=You%20can%20see%20there%20are,alias%2C%20use%20the%20unalias%20command)</sup> At a high level, an alias is just a short-cut so that the user can type in something simple and have something more complex (or more safe) be executed. 

For example, a user could set up:

```mysh> alias ls /bin/ls```

so that within their shell session, the user can simply type ```ls``` and the executable stored in the /bin directory ```/bin/ls``` will be run.

One important thing I wanted to note about aliasing is that alias is an example of a "built-in" command. A built-in command means that my shell interprets this command directly; my shell does not exec() the built-in command and run it as a separate process; instead, the built-in command impacts how my shell itself runs.  

There are three ways that an alias can be invoked in my shell:

1. If the user types the word ```alias```, followed by a single word (the alias-name), followed by a replacement string(s), my shell sets up an alias between the alias-name and the value (e.g. ```alias ls /bin/ls -l -a```). (Special case: If the alias-name was already being used, I just replace the old value with the new value). 

2. If the user just types ```alias```, my shell displays all the aliases that have been set up with one per line.

3. If the user types ```alias``` followed by a word, if the word matches a current alias-name, my shell prints the alias-name and the corresponding replacement value; if the word does not match a current alias-name, it just continues execution.

The user can also unalias alias-names; if the user types ```unalias alias-name``` my shell removes the alias from its list. If <i>alias-name</i> does not exist as an alias, it will just continue. If the user does not specify <i>alias-name</i> or there are too many arguments to unalias my shell prints <i>unalias: Incorrect number of arguments.</i> and continues execution.

Another important thing I wanted to note: there are three words that cannot be used as alias-names: alias, unalias, and exit. For example, if the user types ```alias alias some-string```, ```alias unalias some-string```, or ```alias exit some-string```, my shell prints to stderr <i>alias: Too dangerous to alias that.</i> and continues execution.<br>

To actually use an alias, the user can just type the alias as they would type any other command:<br>
```
mysh> alias ls /bin/ls -l
mysh> ls
```
Note: Currently, running an alias with additional arguments (e.g. ```ls -a``` where ls is an alias-name) is undefined behavior. I have not configured this functionality, so I require that all alias calls consist of only the alias-name. Obviously, you can just create a different alias with the optional argument and there would be no need to use optional arguments with an alias.

## Executing Commands

```int execv(const char *pathname, char *const argv[]);```
<ul>
  <li>The char *const argv[] argument is an array of pointers to null-terminated strings that represent the argument list available to the new program.  The first argument, by convention, should point to the filename associated with the file being executed.  The array of pointers must be terminated by a null pointer.</li>

</ul>  
  
# Running my shell locally
My shell can be invoked by downloading this repository and executing the following command

```./mysh [batch-file]```

The command line argument to my shell can be interpreted as follows:
<ul>
   <li>batch-file: an optional argument. If present, my shell will read each line of the batch-file for commands to be executed. If not present, my shell will run in interactive mode by printing a prompt to the user at stdout and reading the command from stdin.</li>
</ul>

For example, if a user runs my shell as

```./mysh file1.txt```

then my shell will read commands from file1.txt until it sees the exit command.<br>

The following cases are considered errors; in each case, my shell will print a message using ```write()``` to STDERR_FILENO and exit with a return code of 1:
<ul>
   <li>An incorrect number of command line arguments will result in printing <i>Usage: mysh [batch-file]</i> as an error message.</li>
   <li>The batch file does not exist or cannot be opened will result in printing <i>Error: Cannot open file foo.</i> (assuming the file was named foo) as an error message.</li>
    <li>A command does not exist or cannot be executed will result in printing <i>job: Command not found.</i> (assuming the command was named job) to STDERR_FILENO and this time the shell will continue processing.</li>
</ul>
My shell can also handle the following scenarios, which are not errors:
<ul>
   <li>An empty command line.</li>
   <li>White spaces include tabs and spaces.</li>
   <li>Multiple white spaces on an otherwise empty command line.</li>
   <li>Multiple white spaces between command-line arguments, including before the first command on a line and after the last command.</li>
   <li>Batch file ends without exit command or user types 'Ctrl-D' as a command in interactive mode.</li>
</ul> 

## References
1. [Lifewire](https://www.lifewire.com/what-is-a-command-line-interpreter-2625827): What Is a Command Line Interpreter?
2. [MediaCollege](https://www.mediacollege.com/linux/command/alias.html#:~:text=To%20see%20a%20list%20of,type%20alias%20at%20the%20prompt.&text=You%20can%20see%20there%20are,alias%2C%20use%20the%20unalias%20command.): alias command
