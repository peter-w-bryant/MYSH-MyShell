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
are all errors.  My shell prints: "Redirection misformatted". If the output file cannot be opened for some reason (e.g., the user doesn't have write permission or the name is an existing directory), my shell prints "Cannot write to file foo.txt." In these cases, my shell doesn't execute the command and continues to the next line.

### 2.3) Aliases
At high level, an alias is just a short-cut so that the user can type in something simple and have something more complex (or more safe) be executed.  

For example, a user could set up:

<i>mysh> alias ls /bin/ls</i>

so that within their shell session, the user can simply type ls and the executable /bin/ls will be run.

I think it is important to note that alias is an example of a "built-in" command. A built-in command means that my shell interprets this command directly; my shell does not exec() the built-in command and run it as a separate process; instead, the built-in command impacts how my shell itself runs.  

There are three ways that alias can be invoked in my shell. 
<ul>
<li>If the user types the word alias, followed by a single word (the alias-name), followed by a replacement string(s), my shell sets up an alias between the alias-name and the value (e.g. <i>alias ls /bin/ls -l -a</i>). (Special case: If the alias-name was already being used, I just replace the old value with the new value). If the user just types alias, my shell displays all the aliases that have been set up with one per line (first the alias-name, then a single space, and then the corresponding replacement value, with each token separated by exactly one space).</li>

<li>If the user types alias followed by a word, if the word matches a current alias-name, my shell prints the alias-name and the corresponding replacement value, with each token separated by exactly one space; if the word does not match a current alias-name, it just continues.</li>

<li>In my shell, the user can also unalias alias-names; if the user types unalias <alias-name> my shell removes the alias from its list. If <alias-name> does not exist as an alias, it will just continue. If the user does not specify <alias-name> or there are too many arguments to unalias my shell prints 
"unalias: Incorrect number of arguments." and continues.</li>

<ul>
Important thing I wanted to mention: there are three words that cannot be used as alias-names: alias, unalias, and exit. For example, if the user types "alias alias some-string", "alias unalias some-string", or "alias exit some-string", my shell prints to stderr "alias: Too dangerous to alias that." and continues.
  
To actually use an alias, the user can just type the alias as they would type any other command:
mysh> alias ls /bin/ls -l
mysh> ls
Note: Currently, running an alias with additional arguments (e.g. ls -a where ls is an alias-name) is undefined behavior. I have not configured this functionality, so I require that all alias calls consist of only the alias-name.
