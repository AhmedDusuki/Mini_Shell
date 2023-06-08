
/*
 * CS354: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <glob.h>
#include <time.h>

#include "command.h"

SimpleCommand::SimpleCommand()
{
	// Creat available space for 5 arguments
	_numberOfAvailableArguments = 5;
	_numberOfArguments = 0;
	_arguments = (char **) malloc( _numberOfAvailableArguments * sizeof( char * ) );
}

void
SimpleCommand::insertArgument( char * argument )
{
	if ( _numberOfAvailableArguments == _numberOfArguments  + 1 ) {
		// Double the available space
		_numberOfAvailableArguments *= 2;
		_arguments = (char **) realloc( _arguments,
				  _numberOfAvailableArguments * sizeof( char * ) );
	}
	
	_arguments[ _numberOfArguments ] = argument;

	// Add NULL argument at the end
	_arguments[ _numberOfArguments + 1] = NULL;
	
	_numberOfArguments++;
}

Command::Command()
{
	// Create available space for one simple command
	_numberOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc( _numberOfSimpleCommands * sizeof( SimpleCommand * ) );

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
	_append = 0;
}

void
Command::insertSimpleCommand( SimpleCommand * simpleCommand )
{
	if ( _numberOfAvailableSimpleCommands == _numberOfSimpleCommands ) {
		_numberOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
			 _numberOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}
	
	_simpleCommands[ _numberOfSimpleCommands ] = simpleCommand;
	_numberOfSimpleCommands++;
}

void
Command:: clear()
{
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numberOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}
		
		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}

	if ( _outFile ) {
		free( _outFile );
	}

	if ( _inputFile ) {
		free( _inputFile );
	}

	if ( _errFile ) {
		free( _errFile );
	}

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
}

void
Command::print()
{
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");
	
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		printf("  %-3d ", i );
		for ( int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++ ) {
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ] );
		}
		printf("\n");
	}

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background\n" );
	printf( "  ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
		_inputFile?_inputFile:"default", _errFile?_errFile:"default",
		_background?"YES":"NO");
	printf( "\n\n" );
	
}

void
Command::execute()
{
	// Don't do anything if there are no simple commands
	if ( _numberOfSimpleCommands == 0 ) {
		prompt();
		return;
	}

	// Print contents of Command data structure
	
	print();
	// Add execution here
	// For every simple command fork a new process
	// Setup i/o redirection
	// and call exec
	int defaultin = dup( 0 ); // Default file Descriptor for stdin
	int defaultout = dup( 1 ); // Default file Descriptor for stdout
	int defaulterr = dup( 2 ); // Default file Descriptor for stderr
	int in = defaultin,out = defaultout,err=defaulterr;
	int fdpipe[2];
	int pid;
	
	if (_outFile){
		if (_append){
			out = open(_outFile, O_WRONLY|O_APPEND|O_CREAT,0666);
		}
		else{
			out = open(_outFile, O_WRONLY|O_TRUNC|O_CREAT,0666);
		}
	}
	if (_inputFile){
		in = open(_inputFile, O_RDONLY, 0666);
	}
	if (_errFile){
		err = out = open(_errFile, O_APPEND|O_CREAT, 0666);
	}
	
	int oldOut = in;
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		if ( pipe(fdpipe) == -1) {
			perror( "pipe");
			exit( 2 );
		}
		if (i != _numberOfSimpleCommands - 1){ // pipe from/to next command
			dup2(oldOut,0);
			dup2(fdpipe[1],1);
			dup2(err,2);
		}
		else{
			dup2(oldOut,0);
			dup2(out,1);
			dup2(err,2);
		}
		
		pid = fork();
		if ( pid == -1 ) {
			perror( "fork\n");
			exit( 2 );
		}
		if (pid == 0) {
			//Child
			close(fdpipe[0]);
			close(oldOut);
			close(fdpipe[1]);
			close( in );
			close( out );
			close( err );
			
			glob_t globbuf;
			
			int j = 1;
			for (; j < _simpleCommands[i]->_numberOfArguments;){
				
				if (_simpleCommands[i]->_arguments[j][0]!='-'){
					j++;
					break;
				}
				j++;
			}
			j--;
			globbuf.gl_offs = j;
			int k = j;
			for(; j < _simpleCommands[i]->_numberOfArguments;j++){
				if (j == k){
					glob(_simpleCommands[i]->_arguments[j], GLOB_DOOFFS|GLOB_NOMAGIC, NULL, &globbuf);	
				}
				else{
					glob(_simpleCommands[i]->_arguments[j], GLOB_DOOFFS|GLOB_APPEND|GLOB_NOMAGIC, NULL, &globbuf);	
				}
			}
			
			for (int m=0;m < k; m++){
				globbuf.gl_pathv[m] = _simpleCommands[i]->_arguments[m];
			}
			
			
			execvp(_simpleCommands[i]->_arguments[0],&globbuf.gl_pathv[0]);
			// exec() is not suppose to return, something went wrong
			perror( "exec\n");
			exit( 2 );
		}
		close(fdpipe[1]);
		
		if (_background == 0 || (i != 0 && i != (_numberOfSimpleCommands - 1))){
			if (i == _numberOfSimpleCommands - 1){
				// Restore input, output, and error
				dup2( defaultin, 0 );
				dup2( defaultout, 1 );
				dup2( defaulterr, 2 );

				// Close file descriptors that are not needed
				close(fdpipe[1]);
				close(fdpipe[0]);
				close( defaultin );
				close( defaultout );
				close( defaulterr );
			}
			waitpid(pid, 0, 0);
		}
		oldOut = fdpipe[0];
	}
	
	// Restore input, output, and error
	dup2( defaultin, 0 );
	dup2( defaultout, 1 );
	dup2( defaulterr, 2 );

	// Close file descriptors that are not needed
	close(fdpipe[1]);
	close(fdpipe[0]);
	close( defaultin );
	close( defaultout );
	close( defaulterr );
	
	// Clear to prepare for next command
	clear();

	// Print new prompt
	prompt();
}

// Shell implementation

void
Command::leave(){
	kill(getpid(),SIGTERM);
}

void
Command::sigIntIgnore(int sig){
	printf(" Caught SIGNINT, clearing shell, and not terminating!\n");
	Command::_currentCommand.clear();
	Command::_currentCommand.prompt();
}

void
Command::handler (int sig){
	time_t t;
	time(&t);
	FILE *fp;
	fp = fopen("timelog.txt", "a");
	char str[] = "Child process was terminated at: ";
	fprintf(fp, "%s%s",str,ctime(&t));
	fclose(fp);
}

void
Command::cd(char * dir){
	if (dir == 0){
		dir = getenv("HOME");
	}
	if (chdir(dir)){
		printf("Error! Could not chdir!\n");
	}
	clear();
	prompt();
}

void
Command::prompt()
{
	printf("myshell>");
	fflush(stdout);
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;

int yyparse(void);

int 
main()
{
	signal(SIGINT,Command::sigIntIgnore);
	signal(SIGCHLD,Command::handler);
	Command::_currentCommand.prompt();
	yyparse();
	return 0;
}

