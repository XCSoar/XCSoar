/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef SIMPLELIST_H
#define SIMPLELIST_H

template <class T> class List
{
public:
  struct Node
  {
    Node(const T& thedata, Node* next=0):next(next), data(thedata) {}
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
