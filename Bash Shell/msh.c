// The MIT License (MIT)
//
// Copyright (c) 2016 Trevor Bakker
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define WHITESPACE " \t\n"	// We want to split our command line up into tokens
  // so we need to define what delimits our tokens.
  // In this case  white space
  // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255	// The maximum command-line size

#define MAX_NUM_ARGUMENTS 11	// Mav shell only supports four arguments

int main()
{
	int IDNums[15] = {};
	char test[15] = {};
	char command_copy[15] = {};
	int command_Num = 0;
	char* command_string = (char*)malloc(MAX_COMMAND_SIZE);
	char Command_History[15][MAX_COMMAND_SIZE] ;





	while (1)
	{

		// Print out the msh prompt
		printf("msh> ");


		// Read the command from the commandline.  The
		// maximum command that will be read is MAX_COMMAND_SIZE
		// This while command will wait here until the user
		// inputs something since fgets returns NULL when there
		// is no input
		while (!fgets(command_string, MAX_COMMAND_SIZE, stdin));




		if (command_Num < 15)									// The purose of this if-else statement is to check if 15 arguments 
		{                                                       // have been written, if so, we shift the arguments back by one in both
			strcpy(Command_History[command_Num], command_string); // the history array and PID array
		}
		else
		{
			for (int i = 0; i <= 13; i++)
			{
				strcpy(Command_History[i], Command_History[i + 1]); // Since the array has values from index 0 to index 14, we set the
				IDNums[i] = IDNums[i + 1];                          // for loop to 13 to avoid copying random material
			}
			strcpy(Command_History[14], command_string);
		}

		command_Num++;                            // We increase the command counter to check if our command and PID array
                                                 // are full before we shift
		/* Parse input */
		char* token[MAX_NUM_ARGUMENTS];


		for (int i = 0; i < MAX_NUM_ARGUMENTS; i++)
		{

			token[i] = NULL;

		}

		int token_count = 0;

		// Pointer to point to the token
		// parsed by strsep
		char* argument_ptr = NULL;

		char* working_string = strdup(command_string);


		// we are going to move the working_string pointer so
		// keep track of its original value so we can deallocate
		// the correct amount at the end
		char* head_ptr = working_string;

		// Tokenize the input strings with whitespace used as the delimiter
		while (((argument_ptr = strsep(&working_string, WHITESPACE)) != NULL)
			&& (token_count < MAX_NUM_ARGUMENTS))
		{
			token[token_count] = strndup(argument_ptr, MAX_COMMAND_SIZE);
			if (strlen(token[token_count]) == 0)
			{
				token[token_count] = NULL;
			}
			token_count++;
		}


		// Now print the tokenized input as a debug check
		// \TODO Remove this for loop and replace with your shell functionality
		/*
		int token_index  = 0;
		for( token_index = 0; token_index < token_count; token_index ++ )
		{
		  printf("token[%d] = %s\n", token_index, token[token_index] );



		}
		*/
		

		

		if (token[0]!= NULL)                     // Before we execute any code, we need to check if the user has inputed any blanks
		{										// Therfore, we check if the first token is NULL

		  strcpy(test, token[0]);

		  int num = atoi(&test[1]);           // If the user did not input blank space, we copy the first token to check for
											 // the !n command

		  if (test[0]=='!')
		  	{	

				if(num<15&&num<command_Num)
				{
					token_count = 0;
					char *ptr;

					strcpy(command_copy,strtok(Command_History[num],"\n"));
					ptr = strdup(command_copy);

					while (((argument_ptr = strsep(&ptr, WHITESPACE)) != NULL)
					&& (token_count < MAX_NUM_ARGUMENTS))                       // If true, we use the same while loop to parse the command
					{															// string from history into the different tokens
						token[token_count] = strndup(argument_ptr, MAX_COMMAND_SIZE);
						if (strlen(token[token_count]) == 0)
						{
							token[token_count] = NULL;
						}
						token_count++;
					}

				}
				else
				{
					printf("Command not in history\n");
				}
		  	}
			else if ((strcmp(token[0], "history") == 0))  // If not we check, we check for the history command, which reveals all commands
			{									     // that have been inputed into the bash
				if (command_Num < 15)
				{
					IDNums[command_Num] = -1;       
				}
				else
				{
					IDNums[14] = -1;			// Since this is a built in command, we set the PID as -1
				}

				if (token[1] != NULL)              
				{
					if (command_Num < 15)
					{
						IDNums[command_Num] = -1;
					}
					else
					{
						IDNums[14] = -1;
					}

					if (command_Num < 15)
					{
						for (int i = 0; i < command_Num; i++)     		// If token[1] is not equal to NULL, we assume that -p parameter
						{										// has been called, therfore we list all commands with thier respective PIDS
							printf("%d:%s %d\n", i, Command_History[i], IDNums[i]);
						}

					}
					else
					{
						for (int i = 0; i < 14; i++)        // Depending if we filled our history array or not,we set the for loop to
						{									// different values
							printf("%d:%s %d\n", i, Command_History[i],IDNums[i]);
						}
					}

				}
				else
				{
					if (command_Num < 15)
					{
						for (int i = 0; i < command_Num; i++)
						{

							printf("%d:%s\n", i, Command_History[i]);

						}

					}
					else            // These two for loops are designed to operate when the user only calls history 
					{

						for (int i = 0; i < 14; i++)
						{

							printf("%d:%s\n", i, Command_History[i]);

						}

					}

				}
			}
			else if ((strcmp(token[0], "quit") == 0) || (strcmp(token[0], "exit") == 0))
			{

				exit(0);   // The simpliest of commands, if the user wishes to quit the bash, we test for either "exit" or "quit"

			}
			else if ((strcmp(token[0], "cd")) == 0)
			{

				if (command_Num < 15)
				{

					IDNums[command_Num] = -1;

				}
				else
				{

					IDNums[14] = -1;

				}
				chdir(token[1]);

			}
			else
			{
				pid_t pid = fork();

				if (pid == 0)
				{

					// Notice you can add as many NULLs on the end as you want
					int ret = execvp(token[0], &token[0]); // If the passed in command does not match our built in commands
														 // we let fork and execvp execute the command
					if (ret == -1)
					{


						printf("%s\n", token[0]);

						printf("%s: Command not found.\n", token[0]); // If execvp fails, we send the error message 

						return 0;

					}

				}
				else
				{

					int status;

					wait(&status);


					if (command_Num < 15)
					{

						IDNums[command_Num] = pid;   // We save the value of the parent pid into our PID array

					}
					else
					{

						IDNums[14] = pid;

					}



				}

			}

		}
		





		// Cleanup allocated memory
		for (int i = 0; i < MAX_NUM_ARGUMENTS; i++)
		{

			if (token[i] != NULL)
			{

				free(token[i]);

			}

		}


		free(head_ptr);

	}


	free(command_string);


	return 0;

	// e2520ca2-76f3-90d6-0242ac120003
}

