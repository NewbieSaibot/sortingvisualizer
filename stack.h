#ifndef STACK_H
#define STACK_H

#include <memory.h>
#include <stdlib.h>

typedef struct StackNode {
	void* val;
	struct StackNode* prev;
} StackNode;

typedef struct Stack {
	size_t size;
	size_t elsz;
	StackNode* top;
} Stack;

void stkinit(Stack** st, size_t elsz) {
	(*st) = (Stack*)malloc(sizeof(Stack));

	if ((*st) == NULL) {
		return;
	}

	(*st)->elsz = elsz;
	(*st)->size = 0;
	(*st)->top = NULL;
}

const void* stktop(Stack* st) {
	if (st == NULL || st->size == 0 || st->top == NULL) {
		return NULL;
	}

	return st->top->val;
}

void stkpush(Stack* st, const void* el) {
    if (st == NULL) return;
    StackNode* ntop = (StackNode*)malloc(sizeof(StackNode));
    if (ntop == NULL) return;
    ntop->val = malloc(st->elsz);
    if (ntop->val == NULL) {
        free(ntop);
        return;
    }
    memcpy(ntop->val, el, st->elsz);
    ntop->prev = st->top;
    st->top = ntop;
    st->size++;
}

void stkpop(Stack* st) {
    if (st == NULL || st->size == 0 || st->top == NULL) return;
    StackNode* tmp = st->top;
    st->top = st->top->prev;
    free(tmp->val);
    free(tmp);
    st->size--;
}

void stkclear(Stack* st) {
    while (st != NULL && st->size > 0) stkpop(st);
}

void stkfree(Stack* st) {
	if (st == NULL) {
		return;
	}
	stkclear(st);
	free(st);
	st = NULL;
}

#endif // STACK_H
