
#ifndef SOI_MONIORY_BUFFER_H
#define SOI_MONIORY_BUFFER_H

#include "monitor.h"

class Buffer : private Monitor
{
    private:
        class Node
        {
        public:
            int value;
            Node* next;
            Node* previous;

            Node()
            {
                value = 0;
                next = nullptr;
                previous = nullptr;
            }

        };

        Node* head;
        Node* tail;

        size_t actualSize;
        size_t maxSize;

        Condition full;
        Condition empty;

        void addToBuffer(int message);
        int getFromBuffer();
        char bufferID;
    public:
        explicit Buffer(char ID = 'A', size_t maxSize = 1);
        void add(int message, int id);
        int get(int id);
        ~Buffer();
        void setID(char ID);
        char getID();
        unsigned int getMaxSize();
        unsigned int getActualSize();
};


#endif //SOI_MONIORY_BUFFER_H
