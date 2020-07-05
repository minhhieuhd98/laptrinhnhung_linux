#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define MAXPENDING 10 /* Maximum outstanding connection requests */
#define MAXBUFSIZE 1024
#define MAXQUEST 10
#define PROC_OK 0
#define PROC_NG -1

void doIt(int clntSocket);
void deliveryMng(int socket, char buffer[MAXBUFSIZE]);
int dashboardDeliveryAdmin(char productName[MAXBUFSIZE]);
void saveToDeliveryLog(char productName[MAXBUFSIZE]);

int main(int argc, char *argv[])
{
  int servSock;
  int clntSock;
  struct sockaddr_in servAddr;
  struct sockaddr_in clntAddr;
  unsigned short servPort;
  unsigned int clntLen;
  char sendBuffer[MAXBUFSIZE];
  pid_t processID;
  unsigned int childProcCount = 0;

  if (argc != 2)
  {
    fprintf(stderr, "Usage:  %s <Server Port>\n", argv[0]);
    exit(1);
  }

  servPort = atoi(argv[1]);

  if ((servSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
  {
    perror("socket() failed");
    exit(1);
  }
  memset(&servAddr, 0, sizeof(servAddr));
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servAddr.sin_port = htons(servPort);

  if (bind(servSock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
  {
    perror("bind() failed");
    exit(1);
  }

  if (listen(servSock, MAXPENDING) < 0)
  {
    perror("listen() failed");
    exit(1);
  }

  while (1)
  {
    clntLen = sizeof(clntAddr);
    if ((clntSock = accept(servSock, (struct sockaddr *)&clntAddr,
                           &clntLen)) < 0)
    {
      perror("accept() failed");
      exit(1);
    }
    if ((processID = fork()) < 0)
    {
      perror("accept() failed");
      exit(1);
    }
    else if (processID == 0)
    {                  /* If this is the child process */
      close(servSock); /* Child closes parent socket */
      printf("Handling client %s\n", inet_ntoa(clntAddr.sin_addr));
      doIt(clntSock);
      exit(0);
    }

    printf("with child process: %d\n", (int)processID);
    close(clntSock);  /* Parent closes child socket descriptor */
    childProcCount++; /* Increment number of outstanding child processes */

    while (childProcCount)
    {                                                /* Clean up all zombies */
      processID = waitpid((pid_t)-1, NULL, WNOHANG); /* Non-blocking wait */
      if (processID < 0)
      { /* waitpid() error? */
        perror("waitpid() failed");
        exit(1);
      }
      else if (processID == 0) /* No zombie to wait on */
        break;
      else
        childProcCount--; /* Cleaned up after a child */
    }
  }
}

void doIt(int clntSocket)
{
  char buffer[MAXBUFSIZE];
  int recvMsgSize;

  if ((recvMsgSize = recv(clntSocket, buffer, MAXBUFSIZE, 0)) < 0)
  {
    perror("recv() failed");
    exit(1);
  }

  while (recvMsgSize > 0)
  {

    buffer[recvMsgSize] = '\0';
    if (strcmp(buffer, "quit") == 0 || strcmp(buffer, "Quit") == 0)
    {
      close(clntSocket);
    }
    pid_t pid = 0;
    int i, status = 0;

    // Creating child process
    pid = fork();
    if (pid == 0)
      pid = fork();
    switch (pid)
    {
    case -1:
      // Process creating error processing
      perror("processGenerate fork");
      exit(1);
    case 0:
      // Child process processing
      deliveryMng(clntSocket, buffer);

    default:
      // Parent process processing
      // Wait for the state transition of the child process.
      pid = wait(&status);
      printf("pid=%d,status=%d\n", pid, status);
      break;
    }

    if ((recvMsgSize = recv(clntSocket, buffer, MAXBUFSIZE, 0)) < 0)
    {
      perror("recv() failed");
      exit(1);
    }
  }
}

void deliveryMng(int socket, char buffer[MAXBUFSIZE])
{

  char *temp, user[20];
  int i;
  printf("Delivery: %s\n", buffer);
  temp = strtok(buffer, " ");

  if (strcmp(temp, "request") == 0 || strcmp(temp, "Over") == 0)
  {
    temp = strtok(NULL, " ");
    printf("Repling %s starting\n", temp);
    int selected = dashboardDeliveryAdmin(temp);
    if (selected == 1)
      saveToDeliveryLog(temp);
    char mess[15];
    selected == 1 ? strcpy(mess, "deliveried") : strcpy(mess, "NotDelivery");
    mess[strlen(mess)] = '\0';
    selected == 1 ? send(socket, mess, 10, 0) : send(socket, mess, 12, 0);
    printf("Send to socket %d, message %d. Reply end\n", socket, selected);
  }
}

int dashboardDeliveryAdmin(char productName[MAXBUFSIZE])
{
  printf("For %s solded out! Please supply!!\n", productName);
  printf("Enter select:\n");
  printf("1. OK\n");
  printf("0. NO\n");
  int choose;
  scanf("%d", &choose);
  return choose;
}

void saveToDeliveryLog(char productName[MAXBUFSIZE])
{
  printf("Staring save log delivery\n");
  time_t rawtime;
  struct tm *info;

  time(&rawtime);

  info = localtime(&rawtime);

  FILE *fin, *fout;
  fin = fopen("delivery_log.txt", "a");
  if (fin == NULL)
  {
    printf("File Delivery Log is not exits\n");
    exit(0);
  }
  fprintf(fin, "%s\n", asctime(info));
  fprintf(fin, "%s %s\n", productName, "Deliveried");
  fclose(fin);
}
