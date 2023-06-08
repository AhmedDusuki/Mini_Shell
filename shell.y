
/*
 * CS-413 Spring 98
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%token	<string_val> WORD

%token 	GREAT LESS DOUBLEGREAT NEWLINE PIPE AMPERSAND ERR EXIT CD

%union	{
		char   *string_val;
	}

%{
extern "C" 
{
	int yylex();
	void yyerror (char const *s);
}
#define yylex yylex
#include <stdio.h>
#include "command.h"
%}

%%

goal:	
	commands
	;

commands: 
	command
	| command commands
	;

command:exit_command
	| cd_command
	| simple_command
        ;

exit_command:
	EXIT NEWLINE {
		printf("Goodbye!!\n");
		Command::_currentCommand.leave();
	}
	;
	
cd_command:
	CD NEWLINE {
		Command::_currentCommand.cd(0);
	}
	| CD WORD NEWLINE {
		Command::_currentCommand.cd($2);
	}
	;
	
simple_command:	
	
	command_and_args PIPE {
		printf("   Yacc: Pipe command\n");
	}
	| command_and_args iomodifier_opt_loop AMPERSAND NEWLINE {
		printf("   Yacc: Execute command background\n");
		Command::_currentCommand._background = 1;
		Command::_currentCommand.execute();
	}
	| command_and_args iomodifier_opt_loop NEWLINE {
		printf("   Yacc: Execute command\n");
		Command::_currentCommand.execute();
	}
	| NEWLINE 
	| error NEWLINE { yyerrok; }
	;

command_and_args:
	command_word arg_list {
		Command::_currentCommand.
			insertSimpleCommand( Command::_currentSimpleCommand );
	}
	;

arg_list:
	arg_list argument
	| /* can be empty */
	;

argument:
	WORD {
               printf("   Yacc: insert argument \"%s\"\n", $1);

	       Command::_currentSimpleCommand->insertArgument( $1 );\
	}
	;

command_word:
	WORD {
               printf("   Yacc: insert command \"%s\"\n", $1);
	       
	       Command::_currentSimpleCommand = new SimpleCommand();
	       Command::_currentSimpleCommand->insertArgument( $1 );
	}
	;
	
iomodifier_opt_loop:
	iomodifier_opt iomodifier_opt_loop
	| /* can be empty */ 
	;

iomodifier_opt:
	ERR WORD {
		printf("   Yacc: insert error output \"%s\"\n", $2);
		Command::_currentCommand._errFile = $2;
	}
	| DOUBLEGREAT WORD {
		printf("   Yacc: insert append output \"%s\"\n", $2);
		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._append = 1;
	}
	| GREAT WORD {
		printf("   Yacc: insert output \"%s\"\n", $2);
		Command::_currentCommand._outFile = $2;
	}
	| LESS WORD {
		printf("   Yacc: insert input \"%s\"\n", $2);
		Command::_currentCommand._inputFile = $2;
	}
	;

%%

void
yyerror(const char * s)
{
	fprintf(stderr,"%s", s);
}

#if 0
main()
{
	yyparse();
}
#endif
