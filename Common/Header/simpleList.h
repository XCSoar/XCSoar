#ifndef SIMPLELIST_H
#define SIMPLELIST_H

#include "StdAfx.h"


template <class T> class List
{
public:
  struct Node 
  {
    Node(const T& data, Node* next=0):data(data),next(next) {}
    Node* next;
    T data;
  };

  List() : head(0) {}

  List(const List& L) : head(0)
  {
    // copy in reverse order
    for ( const Node* i = L.begin(); i!= L.end(); i=i->next )
      push_front(i->data);
    reverse();
  }

  void reverse()
  {
    Node* p = 0; Node* i = begin(); Node* n;
    while (i)
      {
	n = i->next;
	i->next = p;
	p = i; i = n;
      }
    head = p;
  }

  void swap(List& x)
  {
    Node* tmp = head; head = x.head; x.head = tmp;
  }

  List& operator=(const List& x)
  {
    List tmp(x);
    swap(tmp);
    return *this;
  }

  ~List() { clear(); }
  void clear() { while (!empty()) pop_front(); }

  bool empty() { return !head; }

  void push_front(const T& x) { 
    Node* tmp = new Node(x,head); head = tmp; 
  }

  void pop_front() { 
    if (head) { Node* tmp = head; head=head->next; delete tmp; }
  }

  void insert_after(Node* x, const T& data)
  {
    Node* tmp = new Node(data, x->next);
    x->next = tmp;
  }

  void erase_after(Node* x)
  {
    Node* tmp = x->next;
    if (tmp)
      {
	x->next = x->next->next;
	delete tmp;
      }
  }

  Node *erase(Node *x){
    if (x == head){
      pop_front();
      return(head);
    } else {
      Node* tmp = head;
      while(tmp){
	if (tmp->next == x){
	  erase_after(tmp);
	  break;
	}
	tmp = tmp->next;
      }
      return(tmp->next);
    }
  }


  T& front() { return head->data; }
  const T& front() const { return head->data; }

  Node* begin() { return head; }
  Node* end() { return 0; }

  const Node* begin() const { return head; }
  const Node* end() const { return 0; }

private:
  Node* head;
};


#endif
