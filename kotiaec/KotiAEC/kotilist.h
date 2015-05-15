#ifndef KOTILIST_H
#define KOTILIST_H

#include <malloc.h>
#include <string.h>

namespace koti
{

template<typename T>
struct list_node
{
    T* pointer;
    list_node* next;
};

template<typename T>
class list: public list_node<T>
{
    typedef list_node<T> _Node;

    _Node* begin;
    _Node* end;
    unsigned int _Size;

public:
    typedef list_node<T>* iterator;
    list();

    void push_back(const T& x);
    T* front();
    unsigned int size();
    void erase_front();

    iterator list_begin()
    {
        return begin;
    }

    iterator list_end()
    {
        if(end != NULL)
            return end->next;

        return NULL;
    }
};

template<typename T>
list<T>::list()
{
    begin = NULL;
    end = NULL;
    _Size = 0;
}

template<typename T>
void list<T>::push_back(const T& x)
{
    T* value = (T*) malloc( sizeof(T) );
    memcpy(value, &x, sizeof(x));

    _Node* value_node = (_Node*) malloc( sizeof(_Node) );
    value_node->pointer = value;
    value_node->next = NULL;

    if(_Size == 0)
        begin = end = value_node;
    else if(_Size == 1)
    {
        end = value_node;
        begin->next = end;
    }
    else
    {
        end->next = value_node;
        end = value_node;
    }

    ++_Size;
}

template<typename T>
T* list<T>::front()
{
    if(_Size > 0)
        return begin->pointer;

    return NULL;
}

template<typename T>
unsigned int list<T>::size()
{
    return _Size;
}

template<typename T>
void list<T>::erase_front()
{
    if(_Size > 0)
    {
        _Node tmp = *begin;
        free(begin);
        begin = NULL;

        free(tmp.pointer);
        if(tmp.next)
            begin = tmp.next;

        --_Size;
        if(_Size == 0)
            end = NULL;
    }
}

}

#endif // KOTILIST_H
