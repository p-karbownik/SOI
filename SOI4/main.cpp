#include <iostream>
#include <thread>

#include "Buffer.h"

unsigned int LEFT_SPECIAL_MESSAGES = 4;

unsigned int* PRODUCT_A_PER_PRODUCER;
unsigned int* PRODUCT_B_PER_PRODUCER;
unsigned int* PRODUCT_C_PER_PRODUCER;
unsigned int* PRODUCTS_PER_CONSUMER;

bool receivedSpecialMessages[4] = {false, false, false, false};

int producerAmount;
int consumerAmount;
int* consumerStatus;
int* producerStatus;

Buffer* bufferA;
Buffer* bufferB;
Buffer* bufferC;
Monitor* mutex;

int produceMessage();
int produceSpecialMessage();
void consumer(int id);
void producer(int id);
int chooseBufferToPutIn(unsigned int produced[], int ID);
int chooseBufferToGetOut(unsigned int got[]);
void sendSpecialMessage(int message, int ID);
std::string decryptSpecialMessage(int message);
void updateProducerStatus(const unsigned int produced[], int ID);
void updateConsumerStatus(const unsigned int got[], int ID);
bool isWorkDone();
char getProductType(int x);
void initialiseStatusArrays();
void setProductsAmountToProduce(bool firstTime = true, unsigned int bufferNumber = 0);

int main() {
    using namespace std;

    srand(time(NULL));
    int bufferSize = 4;
    consumerAmount = 8;
    producerAmount = 10;

    initialiseStatusArrays();

    PRODUCTS_PER_CONSUMER = new unsigned int[3];
    PRODUCTS_PER_CONSUMER[0] = PRODUCTS_PER_CONSUMER[1] = PRODUCTS_PER_CONSUMER[2] = 7;
    PRODUCT_A_PER_PRODUCER = new unsigned int[producerAmount];
    PRODUCT_B_PER_PRODUCER = new unsigned int[producerAmount];
    PRODUCT_C_PER_PRODUCER = new unsigned int[producerAmount];

    setProductsAmountToProduce();

    mutex = new Monitor();

    bufferA = new Buffer('A', bufferSize);
    bufferB = new Buffer('B', bufferSize);
    bufferC = new Buffer('C', bufferSize);

    int consumerID = 0;
    int producerID = 0;

    thread consumers[consumerAmount];
    thread producers[producerAmount];

    printf("Warunki poczatkowe:\nIlosc producentow: %d    \nIlosc konsumentow: %d    \nRozmiar buforow: %d    \nIlosc produktow kazdego typu na konsumenta (A/B/C): %d/%d/%d\n", producerAmount, consumerAmount, bufferSize, PRODUCTS_PER_CONSUMER[0], PRODUCTS_PER_CONSUMER[1], PRODUCTS_PER_CONSUMER[2]);

    printf("Przebieg programu:\n\n\n");

    for(int i = 0; i < consumerAmount; i++)
    {
        consumers[i] = thread(consumer, consumerID);

        consumerID++;
    }

    for(int i = 0; i < producerAmount; i++)
    {
        producers[i] = thread(producer, producerID);

        producerID++;
    }


    while(!isWorkDone())
        sleep(1);

    for(int i = 0; i < consumerAmount; i++)
    {
        consumers[i].join();
    }

    for(int i = 0; i < producerAmount; i++)
    {
        producers[i].join();
    }

    printf("Po zakonczeniu:\nIlosc produktow na konsumenta (A/B/C): %d/%d/%d\nIlosc wyslanych wiadomosci specjalnych: %d ", PRODUCTS_PER_CONSUMER[0], PRODUCTS_PER_CONSUMER[1], PRODUCTS_PER_CONSUMER[2], (4 - LEFT_SPECIAL_MESSAGES));
    //zwalnianie pamieci
    delete bufferA;
    delete bufferB;
    delete bufferC;

    delete mutex;

    delete[] consumerStatus;
    delete[] producerStatus;
    delete[] PRODUCTS_PER_CONSUMER;
    delete[] PRODUCT_A_PER_PRODUCER;
    delete[] PRODUCT_B_PER_PRODUCER;
    delete[] PRODUCT_C_PER_PRODUCER;

    return 0;
}

int produceSpecialMessage()
{

    int randomValue = rand() % 30;

    if(randomValue <= 10)
        randomValue =  10;
    else if(randomValue <= 20)
        randomValue = 20;
    else
        randomValue = 30;

    return (randomValue + LEFT_SPECIAL_MESSAGES);
}

int produceMessage()
{

    sleep(1);

    if(LEFT_SPECIAL_MESSAGES == 0)
        return 0;

    else //losujemy, czy wyprodukowana wiadomosc, bedzie specjalna czy tez nie
    {
        if(rand() % 10 < LEFT_SPECIAL_MESSAGES)
        {
            mutex->enter();
            if(LEFT_SPECIAL_MESSAGES == 0)
                return 0;

            LEFT_SPECIAL_MESSAGES--;

            int msg = produceSpecialMessage();
            mutex->leave();


            return msg;
        }
        else
            return 0;
    }
}

void consumer(int id)
{
    unsigned int got[3] = {0, 0, 0};

    int ID = id;
    int chosenBuffer = -1;
    int message = 40;
    bool finished = false;
    char bfID = 'Z';

    printf("Konsument nr %d rozpoczal prace\n", ID);
    while(!finished) {
        chosenBuffer = chooseBufferToGetOut(got);
        if (chosenBuffer == -1)
        {
            ;
        }

        else if (chosenBuffer == 0)
        {
            printf("Konsument nr %d rozpoczal pobieranie wiadomosci z bufora A\n", ID);
            message = bufferA->get(ID);
            bfID = bufferA->getID();
        }

        else if (chosenBuffer == 1)
        {
            printf("Konsument nr %d rozpoczal pobieranie wiadomosci z bufora B\n", ID);
            message = bufferB->get(ID);
            bfID = bufferB->getID();
        }

        else if (chosenBuffer == 2)
        {
            printf("Konsument nr %d rozpoczal pobieranie wiadomosci z bufora C\n", ID);
            message = bufferC->get(ID);
            bfID = bufferC->getID();
        }

        if (message == 40);
        else if (message == 0) //konsument odebral zwykla wiadomosc
        {
            printf("Konsument nr %d odebral zwykla wiadomosc nr %d z bufora %c\n", ID, got[chosenBuffer] + 1, bfID);

            got[chosenBuffer] += 1;
            sleep(1);
        }
        else //konsument odebral wiadomosc specjalna
        {
            printf("Konsument nr %d odebral wiadomosc specjalna, %s (z bufora %c)\n", ID, decryptSpecialMessage(message).c_str(), bfID);
        }

        message = 40;
        updateConsumerStatus(got, ID);
        finished = isWorkDone();
    }

    printf("Konsument nr %d zakonczyl prace. Odebral zwyklych wiadomosci: z bufora A: %d, z bufora B: %d, z bufora C: %d\n", ID, got[0], got[1], got[2]);
}

void producer(int id)
{
    unsigned int produced[3] = {0, 0, 0};
    int ID = id;
    int message = 40;
    int chosenBuffer = -1;
    bool finished = false;
    printf("Producent nr %d rozpoczal prace\n", ID);
    while(!finished)
    {
        if(PRODUCT_A_PER_PRODUCER[ID] != produced[0] | PRODUCT_B_PER_PRODUCER[ID] != produced[1] || PRODUCT_C_PER_PRODUCER[ID] != produced[2])
            message = produceMessage();

        if(message == 40)
        {
            ;
        }

        else if(message == 0) //zwykla wiadomosc
        {
            chosenBuffer = chooseBufferToPutIn(produced, ID);

            if (chosenBuffer == 0) //wkladamy do bufera A
            {
                printf("Producent nr %d rozpoczal wstawianie wiadomosci zwyklej do bufora A\n", ID);
                bufferA->add(message, ID);
                produced[0] += 1;
            }

            else if (chosenBuffer == 1) // wkladamy do bufera B
            {
                printf("Producent nr %d rozpoczal wstawianie wiadomosci zwyklej do bufora B\n", ID);
                bufferB->add(message, ID);
                produced[1] += 1;
            }

            else if (chosenBuffer == 2)//wkladamy do bufera C
            {
                printf("Producent nr %d rozpoczal wstawianie wiadomosci zwyklej do bufora C\n", ID);
                bufferC->add(message, ID);
                produced[2] += 1;
            }

        }

        else //wiadomosc specjalna
        {
            sendSpecialMessage(message, ID);
        }

        updateProducerStatus(produced, ID);
        //sprawdzamy, czy moze sie zakonczyc
        finished = isWorkDone();

        message = 40;
    }

    printf("Producent nr %d zakonczyl prace. Wlozone zwykle wiadomosci: do bufora A: %d, do bufora B: %d, do bufora C: %d\n", ID, produced[0], produced[1], produced[2]);
}

int chooseBufferToPutIn(unsigned int produced[], int ID)
{
   if(produced[0] == PRODUCT_A_PER_PRODUCER[ID] && produced[1] == PRODUCT_B_PER_PRODUCER[ID] && produced[2] == PRODUCT_C_PER_PRODUCER[ID])
       return -1;

   int randomNumber;

   while(true)
   {
       randomNumber = rand() % 3;

       if(randomNumber == 0 && produced[0] != PRODUCT_A_PER_PRODUCER[ID])
       {
           if((bufferA->getMaxSize() - 1 != bufferA->getActualSize()) || (produced[1] == PRODUCT_B_PER_PRODUCER[ID] && produced[2] == PRODUCT_C_PER_PRODUCER[ID]))
               return 0;
           else
               continue;
       }

       if(randomNumber == 0)
           continue;

       if(randomNumber == 1 && produced[1] != PRODUCT_B_PER_PRODUCER[ID])
       {
           if((bufferB->getMaxSize() - 1 != bufferB->getActualSize()) || (produced[0] == PRODUCT_A_PER_PRODUCER[ID] && produced[2] == PRODUCT_C_PER_PRODUCER[ID]))
               return 1;
           else
               continue;
       }

       if(randomNumber == 1)
           continue;

       if(randomNumber == 2 && produced[2] != PRODUCT_C_PER_PRODUCER[ID])
       {
           if((bufferC->getMaxSize() - 1 != bufferC->getActualSize()) || (produced[0] == PRODUCT_A_PER_PRODUCER[ID] && produced[1] == PRODUCT_B_PER_PRODUCER[ID]))
               return 2;
           else
               continue;
       }

       return -1;
   }
}

int chooseBufferToGetOut(unsigned int got[])
{
    if(got[0] == PRODUCTS_PER_CONSUMER[0] && got[1] == PRODUCTS_PER_CONSUMER[1] && got[2] == PRODUCTS_PER_CONSUMER[2])
        return -1;

    int randomNumber;

    while(true)
    {
        randomNumber = rand() % 3;

        if(randomNumber == 0 && got[0] != PRODUCTS_PER_CONSUMER[0])
        {
            if(bufferA->getActualSize() != 0 || ((got[1] == PRODUCTS_PER_CONSUMER[1]) && (got[2] == PRODUCTS_PER_CONSUMER[2])))
                return 0;
            else
                continue;
        }

        if(randomNumber == 0)
            continue;

        if(randomNumber == 1 && got[1] != PRODUCTS_PER_CONSUMER[1])
        {
            if(bufferB->getActualSize() != 0 || ((got[0] == PRODUCTS_PER_CONSUMER[0]) && (got[2] == PRODUCTS_PER_CONSUMER[2])))
                return 1;
            else
                continue;
        }

        if(randomNumber == 1)
            continue;

        if(randomNumber == 2 && got[2] != PRODUCTS_PER_CONSUMER[2])
        {
            if(bufferC->getActualSize() != 0 || ((got[0] == PRODUCTS_PER_CONSUMER[0]) && (got[1] == PRODUCTS_PER_CONSUMER[1])))
                return 2;
            else
                continue;
        }

        return -1;
    }
}

void sendSpecialMessage(int message, int ID)
{
    bufferA->add(message, ID);
    bufferB->add(message, ID);
    bufferC->add(message, ID);
}

std::string decryptSpecialMessage(int message)
{
    mutex->enter();

    std::string returnMessage;

    int productType = message / 10;
    int messageNumber = message % 10;

    if(!receivedSpecialMessages[messageNumber])
    {
        receivedSpecialMessages[messageNumber] = true;

        if(productType == 1)
        {
            PRODUCTS_PER_CONSUMER[0] += 1;

            setProductsAmountToProduce(false, 0);
        }

        else if (productType == 2)
        {
            PRODUCTS_PER_CONSUMER[1] += 1;

            setProductsAmountToProduce(false, 1);
        }

        else if (productType == 3)
        {
            PRODUCTS_PER_CONSUMER[2] += 1;

            setProductsAmountToProduce(false, 2);
        }

        returnMessage = "wiadomosc: " + std::to_string(message) + " pierwszy raz odebrana i zwiekszyla produkcje produktu typu: " + getProductType(productType);
    }

    else
    {
        returnMessage = "wiadomosc: " + std::to_string(message) + "została juz odebrana";
    }

    mutex->leave();
    return returnMessage;
}

void updateProducerStatus(const unsigned int produced[], int ID)
{
    if(produced[0] != PRODUCT_A_PER_PRODUCER[ID] || produced[1] != PRODUCT_B_PER_PRODUCER[ID] || produced[2] != PRODUCT_C_PER_PRODUCER[ID])
    {
        producerStatus[ID] = 0;
        return;
    }

    producerStatus[ID] = 1;
}

void updateConsumerStatus(const unsigned int got[], int ID)
{
    if(got[0] != PRODUCTS_PER_CONSUMER[0] || got[1] != PRODUCTS_PER_CONSUMER[1] || got[2] != PRODUCTS_PER_CONSUMER[2])
    {
        consumerStatus[ID] = 0;
        return;
    }

    consumerStatus[ID] = 1;
}

bool isWorkDone()
{
    for(int i = 0; i < producerAmount; i++)
    {
        if (producerStatus[i] == 0)
            return false;
    }

    for(int i = 0; i < consumerAmount; i++)
    {
        if(consumerStatus[i] == 0)
            return false;
    }

    return true;
}

char getProductType(int x)
{
    if(x ==1)
        return 'A';
    if(x == 2)
        return  'B';
    else
        return 'C';
}

void initialiseStatusArrays()
{
    consumerStatus = new int[consumerAmount];
    producerStatus = new int[producerAmount];

    for(int i = 0; i < consumerAmount; i++)
        consumerStatus[i] = 0;

    for(int i = 0; i < producerAmount; i++)
        producerStatus[i] = 0;
}

void setProductsAmountToProduce(bool firstTime, unsigned int bufferNumber)
{
    if(firstTime)
    {

        for(int i = 0; i < producerAmount; i++)
        {
            PRODUCT_A_PER_PRODUCER[i] = 0;
            PRODUCT_B_PER_PRODUCER[i] = 0;
            PRODUCT_C_PER_PRODUCER[i] = 0;
        };

        unsigned int basicProductsAmountToProduce = (consumerAmount * PRODUCTS_PER_CONSUMER[0]) / producerAmount;
        unsigned int remainder = (consumerAmount * PRODUCTS_PER_CONSUMER[0]) % producerAmount;

        if(basicProductsAmountToProduce > 0)
            for(int i = 0; i < producerAmount; i++)
            {
                PRODUCT_A_PER_PRODUCER[i] = basicProductsAmountToProduce;
                PRODUCT_B_PER_PRODUCER[i] = basicProductsAmountToProduce;
                PRODUCT_C_PER_PRODUCER[i] = basicProductsAmountToProduce;
            };


        if( remainder > 0)
        {
            for(int i = 0; (i < producerAmount) && (remainder > 0); i++)
            {
                PRODUCT_A_PER_PRODUCER[i] += 1;
                PRODUCT_B_PER_PRODUCER[i] += 1;
                PRODUCT_C_PER_PRODUCER[i] += 1;

                remainder--;
            }
        }
    }

    else
    {
        unsigned int newOrderForEveryProducer = consumerAmount / producerAmount;
        unsigned int remainder = consumerAmount % producerAmount;

        if(bufferNumber == 0)
        {
            if(newOrderForEveryProducer > 0)
                for(int i = 0; i < producerAmount; i++)
                    PRODUCT_A_PER_PRODUCER[i] += 1;;
            if(remainder > 0)
                for(int i = 0; i < producerAmount && remainder > 0; i++)
                {
                    PRODUCT_A_PER_PRODUCER[i] += 1;
                    remainder--;
                }
        }

        else if(bufferNumber == 1)
        {
            if(newOrderForEveryProducer > 0)
                for(int i = 0; i < producerAmount; i++)
                    PRODUCT_B_PER_PRODUCER[i] += 1;;
            if(remainder > 0)
                for(int i = 0; i < producerAmount && remainder > 0; i++)
                {
                    PRODUCT_B_PER_PRODUCER[i] += 1;
                    remainder--;
                }
        }

        else if(bufferNumber == 2)
        {
            if(newOrderForEveryProducer > 0)
                for(int i = 0; i < producerAmount; i++)
                    PRODUCT_C_PER_PRODUCER[i] += 1;;
            if(remainder > 0)
                for(int i = 0; i < producerAmount && remainder > 0; i++)
                {
                    PRODUCT_C_PER_PRODUCER[i] += 1;
                    remainder--;
                }
        }
    }
}