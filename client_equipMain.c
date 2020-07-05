#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define MAXBUF_SIZE 1024
#define MAX_PRODS 10
#define PROC_OK 0
#define PROC_NG -1
#define SHM_KEY 0x1234

typedef struct
{
  char name[20];
  int num;
} Product;
Product productList[6];

typedef struct
{
  int prod_id;
  int ncount;
} SharedMemory;

SharedMemory *shmp;

int checkans;

void equipMain(int sock1, int sock2);
void commoditySales(int choice, int sock);
int beverages();
void chosen(int choice);
float drink_cost(int choice);
void readCommodityVM(int, int sock);
void init();

int main(int argc, char *argv[])
{

  int sockMng, sockDelivery;
  struct sockaddr_in servAddrMng;
  struct sockaddr_in servAddrDelivery;
  unsigned short servPortMng, servPortDelivery;
  char *servIP;

  /*-----------menu--------------*/

  if (argc != 4)
  {
    fprintf(stderr, "Usage: %s <Server IP> <Echo Port>\n", argv[0]);
    exit(1);
  }

  servIP = argv[1];
  servPortMng = atoi(argv[2]);
  servPortDelivery = atoi(argv[3]);
  init();

  if ((sockMng = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
  {
    perror("socket() failed");
    exit(1);
  }

  if ((sockDelivery = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
  {
    perror("socket() failed");
    exit(1);
  }

  memset(&servAddrMng, 0, sizeof(servAddrMng));
  servAddrMng.sin_family = AF_INET;
  servAddrMng.sin_addr.s_addr = inet_addr(servIP);
  servAddrMng.sin_port = htons(servPortMng);

  if (connect(sockMng, (struct sockaddr *)&servAddrMng, sizeof(servAddrMng)) < 0)
  {
    perror("connect() failed");
    exit(1);
  }

  memset(&servAddrDelivery, 0, sizeof(servAddrDelivery));
  servAddrDelivery.sin_family = AF_INET;
  servAddrDelivery.sin_addr.s_addr = inet_addr(servIP);
  servAddrDelivery.sin_port = htons(servPortDelivery);

  if (connect(sockDelivery, (struct sockaddr *)&servAddrDelivery, sizeof(servAddrDelivery)) < 0)
  {
    perror("connect() failed");
    exit(1);
  }
  equipMain(sockMng, sockDelivery);

  return 0;
}

void init()
{
  strcpy(productList[1].name, "pepsi");
  productList[1].num = 5;
  strcpy(productList[2].name, "mirinda");
  productList[2].num = 5;
  strcpy(productList[3].name, "Moutain Dew");
  productList[3].num = 5;
  strcpy(productList[4].name, "RedBull");
  productList[4].num = 5;
  strcpy(productList[5].name, "Coffe");
  productList[5].num = 5;
}

void equipMain(int sock1, int sock2)
{
  int user_at_machine = 1;

  char str[100];

  while (user_at_machine)
  {
    int choice;
    /* in an actual program floats and doubles
       would not be used for monetary values
       as they are not always exact */
    float item_cost;
    float money;
    float change;
    /* display items and get users choice */
    choice = beverages();

    /* display pick */
    chosen(choice);

    /* getting money could be in a function */
    if (choice >= 1 && choice <= 20)
    {
      /* get drinks cost */
      item_cost = drink_cost(choice);

      printf("Enter your money: ");
      scanf(" %f", &money);
      printf("\n");
      change = money - item_cost;

      if (change >= 0)
      {
        printf("ACCEPTED!\n");
        printf("Your change is %.2f\n", change);
        switch (choice)
        {
        case 1:
          strcpy(str, productList[1].name);
          break;
        case 2:
          strcpy(str, productList[2].name);
          break;
        case 3:
          strcpy(str, productList[3].name);
          break;
        case 4:
          strcpy(str, productList[4].name);
          break;
        case 5:
          strcpy(str, productList[5].name);
          break;
        case 99:
          break;
        default:
          printf("Invalid input!\n");
          break;
        };
        pid_t pid = 0;
        int i, status = 0;

        // Creating child process for Server Connect Manager
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
          commoditySales(choice, sock1);
          break;
        default:
          // Parent process processing
          // Wait for the state transition of the child process.
          pid = wait(&status);
          printf("pid=%d,status=%d\nPlease enter or any key press for other service", pid, status);
          break;
        }

        //Creating child process for Server Divery
        productList[choice].num--;
        printf("Product number of %s: %d\n", productList[choice].name, productList[choice].num);
        if (productList[choice].num < 4)
        {
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
            readCommodityVM(choice, sock2);
            break;
          default:
            // Parent process processing
            // Wait for the state transition of the child process.
            pid = wait(&status);
            printf("pid=%d,status=%d\nPlease enter or any key press for other service", pid, status);
            break;
          }
        }
        else if (productList[choice].num == 0)
        {
          readCommodityVM(choice, sock2);
          printf("SOLD OUT!\n");
          printf("The amount you paid is not enough! Rollback transaction...\n");
          beverages();
        }
      }
      else
      {
        printf("NOT ACCEPTED!\n");
        printf("The amount you paid is not enough! Rollback transaction...\n");
        beverages();
      }
      printf("\n");

      /* get newline in stream */
      getchar();
      /* get character to pause */
      getchar();
    }
    else
    {
      /* is user quitting ? */
      if (choice == 99)
      {
        user_at_machine = 0;
      }
    }

  } /* end while */
}

/* display beverages and get pick */
int beverages()
{
  int choice;

  printf("---------------------- \n");
  printf("Name list of beverage: ");
  printf("\n\n");
  printf("1.%s\n", productList[1].name);
  printf("\tCost: 1.00$\n");
  printf("2.%s\n", productList[2].name);
  printf("\tCost: 1.00$\n");
  printf("3.%s\n", productList[3].name);
  printf("\tCost: 1.00$\n");
  printf("4.%s\n", productList[4].name);
  printf("\tCost: 2.00$\n");
  printf("5.%s\n", productList[5].name);
  printf("\tCost: 1.50$\n");
  printf("99. Quit\n");

  printf("Enter your choice: ");
  scanf("%d", &choice);
  return choice;
}

/* display users pick */
void chosen(int choice)
{
  switch (choice)
  {
  case 1:
    printf("You choose %s", productList[1].name);
    printf("\tCost: 1.00$\n");
    break;
  case 2:
    printf("You choose %s", productList[2].name);
    printf("\tCost: 1.99$\n");
    break;
  case 3:
    printf("You choose %s", productList[3].name);
    printf("\tCost: 1.00$\n");
    break;
  case 4:
    printf("You choose %s", productList[4].name);
    printf("\tCost: 2.00$\n");
    break;
  case 5:
    printf("You choose %s", productList[5].name);
    printf("\tCost: 1.50$\n");
    break;
  case 99:
    exit(0);
    break;
  default:
    printf("Invalid input!\n");
    break;
  }
}

/*get drink cost */
float drink_cost(int choice)
{
  float cost = 0;

  switch (choice)
  {
  case 1:
    cost = 1.00;
    break;
  case 2:
    cost = 1.99;
    break;
  case 3:
    cost = 1.00;
    break;
  case 4:
    cost = 2.00;
    break;
  case 5:
    cost = 1.5;
    break;
  default:
    cost = 0.00;
    break;
  }

  return cost;
}

void commoditySales(int choice, int sock)
{
  printf("Starting send request to manegerment server with sock: %d\n", sock);
  char str[50];
  strcpy(str, "choice ");
  switch (choice)
  {
  case 1:
    strcat(str, productList[1].name);
    break;
  case 2:
    strcat(str, productList[2].name);
    break;
  case 3:
    strcat(str, productList[3].name);
    break;
  case 4:
    strcat(str, productList[4].name);
    break;
  case 5:
    strcat(str, productList[5].name);
    break;
  default:
    break;
  }
  printf("%s\n", str);
  send(sock, str, strlen(str), 0);
}

void readCommodityVM(int choice, int sock)
{
  printf("Starting send request to delivery server with sock %d\n", sock);
  char buffer[15], str[100];
  //request for server delivery
  strcpy(str, "request ");
  strcat(str, productList[choice].name);
  send(sock, str, strlen(str), 0);
  //Case Over product
  if (productList[choice].num == 0)
  {
    strcpy(str, "Over ");
    strcat(str, productList[choice].name);
    send(sock, str, strlen(str), 0);
  }

  if (recv(sock, buffer, 15, 0) > 0)
  {
    printf("More product....\n");
    if (strcmp(buffer, "deliveried") == 0)
    {
      printf("\n");
      productList[choice].num += 5;
      // shareMemory(choice, productList[choice].num);
    }
    else
    {
      printf("The product isn't additional...\n");
      beverages();
    }
  }
  else
  {
    beverages();
  }
}

void shareMemory(int choice, int numbers)
{
  int shmid;
  //Shared Memory
  shmid = shmget(SHM_KEY, sizeof(SharedMemory), 0644 | IPC_CREAT);
  if (shmid == -1)
  {
    perror("Shared memory");
  }

  shmp = shmat(shmid, NULL, 0);
  if (shmp == (void *)-1)
  {
    perror("Shared memory attach");
  }

  /* Transfer product of data from buffer to shared memory */
  printf("Writing Process block: %d - Size block: %lu - Addr: %lu\n", choice, sizeof(SharedMemory), shmp);
  shmp->prod_id = choice;
  shmp->ncount = numbers;
  sleep(1);

  if (shmdt(shmp) == -1)
  {
    perror("shmdt");
  }

  if (shmctl(shmid, IPC_RMID, 0) == -1)
  {
    perror("shmctl");
  }
  printf("Read block: %d\n", shmp->prod_id);
  printf("Writing Process: Complete\n");
}
