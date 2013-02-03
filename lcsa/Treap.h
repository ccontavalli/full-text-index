#ifndef TREAP_H
#define TREAP_H

//#include <iostream>
#include <assert.h>
#include "basic.h"

//using namespace std;

/** Treap node */
typedef struct _TreapNode
{
		/** Children of the node */
		struct _TreapNode * left, * right;
		/** Priority and value stored in the node */
		ulong priority, value;
} TreapNode;

/** Treap implementation
* It asumes that the values inserted are unique
*
* @author Francisco Claude F.
*/
class Treap
{
	public:
		/** Root node of the treap */
		TreapNode * root;
		ulong * data, * bit_id;
		ulong n;
		
		/** Creates a new empty treap
		* @param data Array being compressed
		* @param bit_id Bit array 
		* @param n length of the array
		*/
		Treap(ulong *data,ulong *bit_id, ulong n);
		
		/** Destroys the treap and frees the resources used by it */
		~Treap();
		
		/** Insert a pair (value,priority) into the treap
		* @param value Value to instert
		* @param priority Priority of the value inserted
		*/
		void insert(ulong value, ulong priority);
		
		/** Removes a value from the treap
		* @param value Value to delete from the treap
		*/
		void remove(ulong value);
		
		/** Gets the highest priority element of the treap */
		TreapNode * top();
		
		/** Gets and deletes the highest priority element of the treap */
		TreapNode * pop();
		
		/** Searches for a specific value 
		* @param value The value to search
		*/
		TreapNode * search(ulong value);

		void decreaseKey(ulong val, ulong prio);
		bool check(char * str="");
		void print();
		
	protected:
		void freeNode(TreapNode * r);
		TreapNode * rinsert(TreapNode * n, TreapNode * r);
		TreapNode * rinsert2(TreapNode * n, TreapNode * r, bool *rota);
		TreapNode * rdecreaseKey(TreapNode * r, ulong val, ulong prio);
		TreapNode * rsearch(ulong value, TreapNode * r);
		TreapNode * rootRemove(TreapNode * r);
		TreapNode * rootDown(TreapNode * r);
		TreapNode * rremove(ulong value, TreapNode * r);
		TreapNode * rotateLeft(TreapNode * r);
		TreapNode * rotateRight(TreapNode * r);
		void rprint(TreapNode * r);
		long compare(ulong v1, ulong v2);
		bool sw(ulong v1, ulong v2);
		long sw2(ulong v1, ulong v2);
		bool rcheck(TreapNode * r);
};

#endif
