
#include "Treap.h"

#define CHECK  0

static ulong n3;
// Public methods
Treap::Treap(ulong * _data,ulong * _bit_id, ulong _n)
{
  root = NULL;
  data = _data;
  bit_id = _bit_id;
  n = _n;
  n3 = bits(n+1)/2;//(1 << (bits(n+1)/2))-1;
}

Treap::~Treap()
{
  freeNode(root);
}

void Treap::insert(ulong _value, ulong _priority)
{
  TreapNode * n = (TreapNode *)malloc(sizeof(TreapNode));
  n->left = NULL;
  n->right = NULL;
  n->value = _value;
  n->priority = _priority;
  bool rota=true;
  root = rinsert2(n, root,&rota);
  //root = rinsert(n, root);
  #if CHECK==1
    //check("Insert");
  #endif
}

void Treap::remove(ulong value)
{
  root = rremove(value,root);
  #if CHECK==1
    check("Remove");
  #endif
}

TreapNode * Treap::top()
{
  return root;
}

TreapNode * Treap::pop()
{
  if (root == NULL) return NULL;
  TreapNode * n = (TreapNode *)malloc(sizeof(TreapNode));
  n->value= root->value;
  n->priority= root->priority;
  root=rremove(n->value,root);
  #if CHECK==1
    check("Pop");
  #endif
  return n;
}

TreapNode * Treap::search(ulong value)
{
  TreapNode *r;
  r=root;
  //return rsearch(value, root);
  if(r==NULL) return NULL;
  long cmp = compare(r->value,value);
  while(cmp!=0) {
    if(cmp<0) r=r->right;
    else if(cmp>0) r=r->left;
    if(r==NULL) return NULL;
    cmp = compare(r->value,value);
  }
  return r;
}

void Treap::print()
{
  rprint(root);
  printf("\n");
}

// Protected methods
void Treap::freeNode(TreapNode * r)
{
  if(r==NULL) return;
  //printf("r=0x%x\n",r); fflush(stdout);
  freeNode(r->left);
  freeNode(r->right);
  free(r);
}

/*TreapNode * Treap::rinsert(TreapNode * n, TreapNode * r)
{
        if(r==NULL) return n;
        long cmp = compare(r->value,n->value);
        if(cmp > 0) {
                r->left = rinsert(n, r->left);
                if(r->left->priority > r->priority)
                        r=rotateRight(r);
                //else if(r->left->priority == r->priority && sw(r->left->value, r->value))
                        //r=rotateRight(r);
        }
        else {//if(cmp < 0) {
                r->right = rinsert(n, r->right);
                if(r->right->priority > r->priority)
                        r=rotateLeft(r);
                //else if(r->right->priority == r->priority && sw(r->right->value, r->value))
                        //r=rotateLeft(r);
        }
        return r;
}*/


TreapNode * Treap::rinsert(TreapNode * n, TreapNode * r)
{
  if(r==NULL) return n;
  long cmp = compare(r->value,n->value);
  if(cmp > 0) {
    r->left = rinsert(n, r->left);
    if ((r->left->priority > r->priority) || ((r->left->priority == r->priority) && sw(r->left->value, r->value)))
      r=rotateRight(r);
  }
  else {// if(cmp < 0) {
    r->right = rinsert(n, r->right);
    if((r->right->priority > r->priority) || (r->right->priority == r->priority && sw(r->right->value, r->value)))
      r=rotateLeft(r);
  } //else { assert(r->value == n->value); // Repeated value in treap }
  return r;
}

TreapNode * Treap::rinsert2(TreapNode * n, TreapNode * r, bool *rota)
{
  if(r==NULL) return n; 
  long cmp = compare(r->value,n->value);
  if(cmp > 0) {
    r->left = rinsert2(n, r->left,rota);
    if (*rota && ((r->left->priority > r->priority) || ((r->left->priority == r->priority) && sw(r->left->value, r->value))))
      r=rotateRight(r);
    else
      *rota=false;
  }
  else {// if(cmp < 0) {
    r->right = rinsert2(n, r->right,rota);
    if (*rota && ((r->right->priority > r->priority) || (r->right->priority == r->priority && sw(r->right->value, r->value))))
      r=rotateLeft(r);
    else
      *rota=false;
  } //else { assert(r->value == n->value); } // Repeated value in treap 
  return r;
}


TreapNode * Treap::rsearch(ulong value, TreapNode * r)
{
  if(r==NULL) return r;
  long cmp = compare(r->value,value);
  while(cmp!=0) {
    if(cmp<0) r=r->right;
    else if(cmp>0) r=r->left;
    if(r==NULL) return NULL;
    cmp = compare(r->value,value);
  }
  return r;
  /*if(cmp < 0) return rsearch(value,r->right);
  else if(cmp > 0) return rsearch(value,r->left);
  return r;*/
}

TreapNode * Treap::rremove(ulong value, TreapNode * rr)
{
  if(rr==NULL) return rr;
  TreapNode * aux = rr;
  TreapNode * r = rr;
  long cmp = compare(r->value,value);
  while(cmp!=0) {
    aux=r;
    if(cmp<0) r=r->right;
    else if(cmp>0) r=r->left;
    if(r==NULL) return NULL;
    cmp = compare(r->value,value);
  }
  cmp= compare(aux->value,value);
  if(cmp<0)
    aux->right=rootRemove(aux->right);
  else if (cmp>0)
    aux->left=rootRemove(aux->left);
  else return rootRemove(r);
  return rr;
  /*if(r==NULL) return NULL;
  long cmp = compare(r->value,value);
  if(cmp > 0) r->left = rremove(value, r->left);
  else if(cmp < 0) r->right = rremove(value, r->right);
  else r = rootRemove(r);
  return r;*/
}

TreapNode * Treap::rootRemove(TreapNode * r)
{
  if(r==NULL) return NULL;
  if(r->left==NULL && r->right==NULL) { free(r); return NULL;}
  if(r->left == NULL) {
    r = rotateLeft(r);
    r->left = rootRemove(r->left);
  }
  else if(r->right == NULL) {
    r = rotateRight(r);
    r->right = rootRemove(r->right);
  }
  else if(r->left->priority > r->right->priority || (r->left->priority==r->right->priority && sw(r->left->value, r->right->value))) {
    r=rotateRight(r);
    r->right = rootRemove(r->right);
  }
  else {
    r=rotateLeft(r);
    r->left = rootRemove(r->left);
  }
  return r;
}

inline TreapNode * Treap::rotateLeft(TreapNode * r)
{
  TreapNode * newRoot = r->right;
  TreapNode * aux = r->right->left;
  newRoot->left = r;
  r->right = aux;
  return newRoot;
}

inline TreapNode * Treap::rotateRight(TreapNode * r)
{
  TreapNode * newRoot = r->left;
  TreapNode * aux = r->left->right;
  newRoot->right = r;
  r->left = aux;
  return newRoot;
}

void Treap::rprint(TreapNode * r)
{
  if(r==NULL) return;
  else {
    printf("(");
    rprint(r->left);
    printf(")[%lu,%lu](", r->value , r->priority );
    rprint(r->right);
    printf( ")");
  }
}

bool Treap::sw(ulong v1, ulong v2)
{
  /* return false; */
  ulong max1 = data[v1];
  ulong max2 = data[v2];
  v1++; v2++;
  if(!bitget(bit_id,v1))
    v1 += data[v1];
  if(!bitget(bit_id,v2))
    v2 += data[v2];
  max1 = max(data[v1],max1);
  max2 = max(data[v2],max2);
  return max1<max2;
}

long Treap::sw2(ulong v1, ulong v2)
{
  /* return false; */
  ulong max1 = data[v1];
  ulong max2 = data[v2];
  v1++; v2++;
  if(!bitget(bit_id,v1))
    v1 += data[v1];
  if(!bitget(bit_id,v2))
    v2 += data[v2];
  max1 = max(data[v1],max1);
  max2 = max(data[v2],max2);
  return max1-max2;
}

inline long Treap::compare(ulong v1, ulong v2)
{
  /* return v1-v2; */
  ulong aux,aux1, aux2;
  if(!bitget(bit_id,v1+1)) aux1=data[v1+1+ data[v1+1]]; else aux1=data[v1+1];
  if(!bitget(bit_id,v2+1)) aux2=data[v2+1+ data[v2+1]]; else aux2=data[v2+1];

  /* comentar las dos lineas siguientes para dejar como antes */  
  //aux=((data[v1]&n3)*(aux1&n3))-((data[v2]&n3)*(aux2&n3));
  //aux=((data[v1]>>n3)*(aux1>>n3))-((data[v2]>>n3)*(aux2>>n3));
  aux=((data[v1]>>n3)*(aux1>>n3))-((data[v2]>>n3)*(aux2>>n3));
  if (aux != 0) return aux;

  aux=data[v1]-data[v2];
  if(aux!=0) 
    return aux;
  else 
    return aux1-aux2;
}
/*
inline long Treap::compare(ulong v1, ulong v2)
{
  // return v1-v2; 
  if(data[v1]==data[v2]) {
    v1++; v2++;
    if(!bitget(bit_id,v1))
      v1 += data[v1];
    if(!bitget(bit_id,v2))
      v2 += data[v2];
//    if(data[v1]==data[v2]) return 0;  
  }
  //return (data[v1]<data[v2])?-1:1;
  return data[v1]-data[v2];
}
*/

void Treap::decreaseKey(ulong val, ulong prio) {
  root = rdecreaseKey(root,val,prio);
  //remove(val);
  //insert(val,prio);
  //  check("Decrease Key");
}

TreapNode * Treap::rootDown(TreapNode * r)
{ 
  //long aux;
  if(r==NULL || (r->left==r->right && r->left==NULL)) return r;
  if(r->left == NULL) {
    if(r->right->priority<r->priority || (r->right->priority==r->priority && sw(r->value,r->right->value))) 
      return r;
    r = rotateLeft(r);
    r->left = rootDown(r->left);
  }
  else if(r->right == NULL) {
    if(r->left->priority<r->priority || (r->left->priority==r->priority && sw(r->value,r->left->value))) 
      return r;
    r = rotateRight(r);
    r->right = rootDown(r->right);
  }
  else if(r->priority<max(r->left->priority,r->right->priority) || (r->priority==r->left->priority && sw(r->left->value,r->value)) || (r->priority==r->right->priority && sw(r->right->value,r->value))) {
    if(r->left->priority > r->right->priority || (r->left->priority==r->right->priority && sw(r->left->value, r->right->value))) {
      r=rotateRight(r);
      r->right = rootDown(r->right);
    }
    else { //if(r->right->priority > r->left->priority || (r->right->priority==r->left->priority && sw(r->right->value, r->left->value))) {
      r=rotateLeft(r);
      r->left = rootDown(r->left);
    }
  }

  return r;
}

TreapNode * Treap::rdecreaseKey(TreapNode * rr, ulong val,ulong prio) {
  if(rr==NULL) return NULL;
  TreapNode * r = rr;
  TreapNode * aux = r;
  long cmp = compare(r->value,val);
  while(cmp!=0) {
    aux=r;
    if(cmp<0) r=r->right;
    else if(cmp>0) r=r->left;
    if(r==NULL) return NULL;
    cmp = compare(r->value,val);
  }
  r->priority=prio;
  r->value=val;
  cmp= compare(aux->value,val);
  if(cmp<0)
    aux->right=rootDown(r);
  else if (cmp>0)
    aux->left=rootDown(r);
  else 
    rr=rootDown(r);
  return rr;
/*
  if(rr==NULL) return NULL;

  long cmp = compare(val, rr->value);
  if(cmp<0) {
    rr->left = rdecreaseKey(rr->left,val,prio);
  }
  else if(cmp>0) {
    rr->right = rdecreaseKey(rr->right,val,prio);
  }
  else {
    rr->priority=prio;
    rr->value=val;
    rr=rootDown(rr);
  }
  return rr;
*/
}

bool Treap::check(char * str) {
  printf("Check %s:\n",str);
  assert(rcheck(root));
  return true;
}

bool Treap::rcheck(TreapNode * r) {
  bool ret = true;
  if(r->left!=NULL) {
    if(r->left->priority>r->priority || compare(r->value,r->left->value)<0) {
      printf("%lu esta bajo %lu\n",r->left->priority,r->priority);
      return false;
    }
    ret &= rcheck(r->left);
  }
  if(r->right!=NULL) {
    if(r->right->priority>r->priority || compare(r->value,r->right->value)>0) {
      printf("%lu esta bajo %lu\n",r->right->priority,r->priority);
      return false;
    }
    ret &= rcheck(r->right);
  }
  return ret;
}

