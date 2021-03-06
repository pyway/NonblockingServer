/***************************************************************
 *
 * client.c
 * The main module for a simple chat client written in C.
 * It uses non-blocking sockets and non-blocking terminal input.
 *
 * Client appears to die gracefully when disconnected from the
 * server.  The reason is unclear to the author.
 *
 ***************************************************************
 *
 * This software was written in 2013 by the following author(s):
 * Brendan A R Sechter <bsechter@sennue.com> 
 *
 * To the extent possible under law, the author(s) have
 * dedicated all copyright and related and neighboring rights
 * to this software to the public domain worldwide. This
 * software is distributed without any warranty.
 *
 * You should have received a copy of the CC0 Public Domain
 * Dedication along with this software. If not, see
 * <http://creativecommons.org/publicdomain/zero/1.0/>.
 *
 * Please release derivative works under the terms of the CC0
 * Public Domain Dedication.
 *
 ***************************************************************/

// library headers
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// module headers
#include "args.h"
#include "client.h"
#include "error.h"
#include "message.h"
#include "network.h"
#include "terminalInput.h"

// act on user input
int executeCommand(int pSocketFile, char *pCommand)
{
  int done = 0;
  if (0 == strcmp(pCommand, MESSAGE_COMMAND_EXIT))
  {
    done = 1;
  }
  else if (0 == strcmp(pCommand, MESSAGE_NULL))
  {
    printf("%s %s\n", terminalInputGetPrompt(), MESSAGE_DEFAULT);
    sendMessage(pSocketFile, MESSAGE_DEFAULT);
  }
  else
  {
    sendMessage(pSocketFile, pCommand);
  }
  terminalInputPrompt();
  return done;
}

// process user input
int processInput(int pSocketFile, char *pBuffer, int pSize)
{
  int done = 0;
  while (terminalInputReady())
  {
    int clearLine = 0;
    int c = fgetc(stdin);
    switch (c)
    {
      case '\n':
        done = executeCommand(pSocketFile, pBuffer);
        break;
      case '\b':
      case 127:
      case 224:
        printf("\n");
        if (clearLine)
        {
          terminalInputPrompt();
        }
        else
        {
          terminalInputBackspace();
          terminalInputPromptDisplay();
        }
        break;
      default:
        terminalInputBufferCharacter(c);
        break;
    }
  }
  return done;
}

// process messages from server and display output
int processOutput(int pSocket, char *pBuffer, int pSize)
{
  int done = 0;
  int outputPrinted = 0;
  while (receiveMessageReady(pSocket))
  {
    if (0 == receiveMessage(pSocket, pBuffer, pSize))
    {
      printf("\n");
      ERROR("Disconnected from server.");
      done = 1;
      break;
    }
    else
    {
      if (!outputPrinted)
      {
        outputPrinted = 1;
        printf("\n");
      }
      printf("Message : %s\n", pBuffer);
    }
  }
  if (outputPrinted)
  {
    terminalInputPromptDisplay();
  }
  return done;
}

// initialize client
int init(client_param_t *pParameters)
{
  // create socket and connect to server
  int socketFile = initClient(pParameters->port, pParameters->host);

  // print host and port
  printf("Host: %s\n", pParameters->host);
  printf("Port: %d\n", pParameters->port);

  // return socket
  return socketFile;
}

// cleanup before exiting
void cleanup(int pSocketFile)
{
  terminalInputCleanUp();
  close(pSocketFile);
  printf("\n");
}

// service loop
int service(int pSocketFile)
{
  char inputBuffer [NETWORK_COMMUNICATION_BUFFER_SIZE];
  char outputBuffer[NETWORK_COMMUNICATION_BUFFER_SIZE];
  int  done = 0;

  // initialize service loop
  terminalInputInit(TERMINAL_INPUT_DEFAULT_PROMPT, inputBuffer, sizeof(inputBuffer));
  terminalInputPromptDisplay();

  // main loop
  while (!done)
  {
    done += processOutput(pSocketFile, outputBuffer, sizeof(outputBuffer));
    done += processInput (pSocketFile, inputBuffer,  sizeof(inputBuffer));
  }

  return EXIT_SUCCESS;
}

// program usage
int usage(int pArgC, char *pArgV[], int pArgN, args_param_t *pArgsParam, void *pData)
{
  printf("Usage: %s [params]\n", pArgV[0]);
  printf("    -p     <port number>\n"    );
  printf("    --port <port number>\n"    );
  printf("        Set port number.\n"    );
  printf("    -h     <host name>\n"      );
  printf("    --host <host name>\n"      );
  printf("        Set host.\n"           );
  printf("    -?\n"                      );
  printf("    --help\n"                  );
  printf("        Print this usage.\n"   );
  exit(EXIT_SUCCESS);
  return 1;
}

// main program
int main(int argc, char *argv[])
{
  client_param_t parameters =
  {
    NETWORK_DEFAULT_PORT,
    NETWORK_DEFAULT_HOST
  };
  args_param_t args_param_list[] =
  {
    {"-p",     &parameters.port, argsInteger},
    {"--port", &parameters.port, argsInteger},
    {"-h",     &parameters.host, argsString },
    {"--host", &parameters.host, argsString },
    {"-?",     NULL,             usage      },
    {"--help", NULL,             usage      },
    ARGS_DONE
  };
  int socketFile;
  int result;

  // process command line arguments
  argsProcess(argc, argv, args_param_list);

  // initialize program
  socketFile = init(&parameters);

  // run service loop
  result = service(socketFile);

  // cleanup
  cleanup(socketFile);

  // exit
  return result;
}

