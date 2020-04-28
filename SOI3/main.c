#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <time.h>
#include <sys/sem.h>
#include <sys/shm.h>

#define SEMAPHORES_AMOUNT 3
#define EMPTY_SEMAPHORE_ID 0
#define MUTEX_ID 1
#define FULL_SEMAPHORE_ID 2

int semID = 0;
int shmID = 0;
int stopped = 0;
int fControlIndex = 0;
int sControlIndex = 0;

int* communicationBuffer;

static struct sembuf buf;

int initSemaphores(int MAX_SIZE);
void up_semaphore(int x);
void down_semaphore(int x);

int produceItem();
void addItem(int anItem);
int getItem();

void producer(int productAmount, int id);
void consumer(int productAmount, int id);

void CreateProcess(void (*aFunction)(int, int), int a, int b);

int main(int argc, char* argv[]) {

    if(argc != 5)
    {
        printf("Nie poprawna ilosc danych");
        return 1;
    }

    int consumersAmount;
    int producersAmount;
    int bufferSize;
    int i;
    int productsAmountForProducer;

    consumersAmount = atoi(argv[1]);
    producersAmount = atoi(argv[2]);
    productsAmountForProducer = atoi(argv[3]);
    bufferSize = atoi(argv[4]);

    if(consumersAmount <= 0 || producersAmount <= 0 || productsAmountForProducer < 0 || bufferSize <= 0)
    {
        printf("Zła wartość podanych danych");
        return 1;
    }

    if (initSemaphores(bufferSize) == 1)
    {
        printf("Blad initSemaphores");
        return 1;
    }

    //alokacja wspolnej pamieci

    shmID = shmget(IPC_PRIVATE, bufferSize * sizeof(int) + 2 * sizeof(int), IPC_CREAT | 0600);

    if (shmID == -1)
    {
        printf("Blad tworzenia wspolnej pamieci");
        return 1;
    }

    communicationBuffer = (int*) shmat(shmID, NULL, 0);

    fControlIndex = bufferSize;
    sControlIndex = bufferSize + 1;
    int productsAmountForOneConsumer = (producersAmount * productsAmountForProducer) / consumersAmount;
    int productsForFirstConsumer = productsAmountForOneConsumer + (productsAmountForProducer % consumersAmount);

    for(i = 0; i < producersAmount; i++)
    {
        CreateProcess((void*) *producer, productsAmountForProducer, i);
    }

    for(i = 0; i < consumersAmount; i++)
    {
        if(i == 0)
            CreateProcess((void*) *consumer, productsForFirstConsumer, i);
        else
            CreateProcess((void*) *consumer, productsAmountForOneConsumer, i);
    }

    while(communicationBuffer[sControlIndex] < (consumersAmount + producersAmount))
    {
    }

    shmctl(shmID, IPC_RMID, NULL);

    return 0;
}

int initSemaphores(int MAX_SIZE)
{
    semID = semget(IPC_PRIVATE, SEMAPHORES_AMOUNT, IPC_CREAT | 0600);

    if(semID == -1)
        return 1;

    else
    {
        if(semctl(semID, EMPTY_SEMAPHORE_ID, SETVAL, (int) MAX_SIZE) == -1)
            return 1;
        if(semctl(semID, MUTEX_ID, SETVAL, (int) 1) == -1)
            return 1;
        if(semctl(semID, FULL_SEMAPHORE_ID, SETVAL, (int) 0) == -1)
            return 1;
    }

    return 0;
}

void up_semaphore(int x)
{
    buf.sem_num = x;
    buf.sem_op = 1;
    buf.sem_flg = 0;

    if(semop(semID, &buf, 1) == -1)
    {
        exit(1);
    }
}

void down_semaphore(int x)
{
    buf.sem_num = x;
    buf.sem_op = -1;
    buf.sem_flg = 0;

    if(semop(semID, &buf, 1) == -1)
    {
        exit(1);
    }
}

int produceItem()
{
    int i, seed;
    time_t aTime;

    seed = time(&aTime);
    srand(seed);

    i = rand() % 100;

    if(i > 20)
        return 10;
    else
        return 20;
}

void addItem(int anItem)
{
    int i;
    int size = fControlIndex;

    for(i = 0; i < size; i++)
    {
        if(communicationBuffer[i] == 0)
        {
            communicationBuffer[i] = anItem;
            communicationBuffer[fControlIndex] = i;
            return;;
        }
    }
}

int getItem() {
    int theItem = 0;
    int k = 0;
    int i;
    int size = fControlIndex;
    //szukanie produktow premium

    for (i = 0; i < fControlIndex; i++)
    {
        if(communicationBuffer[i] == 20)
        {
            k = i;
            theItem = communicationBuffer[i];
            communicationBuffer[i] = 0;
            break;
        }
    }

    if(theItem != 0)
    {
        for (; k < size; k++) {
            if (k + 1 < size) {
                communicationBuffer[k] = communicationBuffer[k + 1];
            }
        }

        communicationBuffer[size - 1] = 0;
    }
    else
    {
        for(i = size - 1; i >= 0; i--)
        {
            if(communicationBuffer[i] != 0)
            {
                theItem = communicationBuffer[i];
                communicationBuffer[i] = 0;
                break;
            }
        }
    }
    communicationBuffer[size]--;
    return theItem;
}

void producer(int productAmount, int id)
{
    int i = productAmount;
    int temp_product = 0;
    communicationBuffer = (int*) shmat(shmID, NULL, 0);

    printf("Producent numer: %d rozpoczal prace\n", id);

    while (i > 0)
    {
        sleep(1);
        temp_product = produceItem();
        down_semaphore(EMPTY_SEMAPHORE_ID);
        down_semaphore(MUTEX_ID);

        addItem(temp_product);
        if (i == 1)
            communicationBuffer[sControlIndex]++;
        up_semaphore(MUTEX_ID);
        up_semaphore(FULL_SEMAPHORE_ID);

        printf("Producent numer: %d dodal produkt o wartosci: %d\n", id, temp_product);

        i--;
    }
    printf("Producent numer: %d zakonczyl prace\n", id);
}

void consumer(int productAmount, int id)
{
    int i = productAmount;

    printf("Konsument numer: %d rozpoczal prace\n", id);

    communicationBuffer = (int*) shmat(shmID, NULL, 0);

    while (i > 0)
    {
        down_semaphore(FULL_SEMAPHORE_ID);
        down_semaphore(MUTEX_ID);

        int element = getItem();
        if (i == 1)
            communicationBuffer[sControlIndex]++;
        up_semaphore(MUTEX_ID);
        up_semaphore(EMPTY_SEMAPHORE_ID);

        printf("Konsument numer: %d odebral produkt o wartosci: %d\n", id, element);
        i--;
    }


    printf("Konsument nr: %d zakonczyl prace\n", id);
}

void CreateProcess(void (*aFunction)(int, int), int a, int b)
{
    int creationResult = fork();
    if(creationResult == 0)
    {
        aFunction(a, b);
        exit(0);
    }
    stopped++;
}

