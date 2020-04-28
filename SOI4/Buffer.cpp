
#include <iostream>
#include "Buffer.h"

Buffer::Buffer(char id, size_t sth)
{
    this->maxSize = sth;
    this->bufferID = id;
    this->head = nullptr;
    this->tail = head;
    this->actualSize = 0;
}

int Buffer::get(int id)
{
    enter();
    if(actualSize == 0)
    {
        printf("Konsument nr %d zostal wstrzymany na zmiennej empty (Bufor %c)\n", id, bufferID);
        wait(empty);
        printf("Konsument nr %d zostal wznowiony po zmiennej empty (Bufor %c)\n", id, bufferID);
    }
    int msg = this->getFromBuffer();

    if(actualSize == maxSize - 1)
        signal(full);

    leave();

    return msg;
}

int Buffer::getFromBuffer()
{
    int message = head->value;

    if(head == tail)
    {
        delete head;
        head = nullptr;
        tail = nullptr;
    }

    else
    {
        Node* tmp = head;
        head = head->next;

        delete tmp;
    }

    actualSize--;

    return message;
}

void Buffer::addToBuffer(int message)
{

    Node* node = new Node();
    node->value = message;

    if(message == 0)
    {
        node->next = nullptr;

        if (head == nullptr && tail == nullptr)
        {
            node->previous = nullptr;

            head = node;
            tail = node;
        }

        else
        {
            node->previous = tail;
            tail->next = node;
            tail = node;
        }
    }

    else
    {
        node->previous = nullptr;

        if (head == nullptr && tail == nullptr)
        {
            node->next = nullptr;

            head = node;
            tail = node;
        }

        else
        {
            node->next = head;
            head->previous = node;
            head = node;
        }
    }
    actualSize++;
}

void Buffer::add(int message, int id)
{
    enter();
    while(actualSize == maxSize)
    {
        printf("Producent nr %d zostal wstrzymany przez full (Bufor: %c)\n", id, bufferID);
        wait(full);
        printf("Producent nr %d zostal wznowiony po full (Bufor %c)\n", id, bufferID);
    }
    this->addToBuffer(message);

    std::string s = "Producent nr " + std::to_string(id);
    s += " wstawil wiadomosc ";
    if (message == 0)
    {
        s += "zwykla do bufora ";
        s += bufferID;
    }
    else
    {
        s += "specjalno o tresci: ";
        s += std::to_string(message);
        s += " do bufora ";
        s += bufferID;
    }

    printf("%s\n", s.c_str());

    if(actualSize == 1)
        signal(empty);
    leave();
}

Buffer::~Buffer()
{
    Node* temp;
    while(head != nullptr)
    {
        temp = head;
        head = head->next;
        delete temp;
    }
}

void Buffer::setID(char ID)
{
    bufferID = ID;
}

char Buffer::getID()
{
    return bufferID;
}

unsigned int Buffer::getMaxSize()
{
    return maxSize;
}

unsigned int Buffer::getActualSize()
{
    return actualSize;
}
