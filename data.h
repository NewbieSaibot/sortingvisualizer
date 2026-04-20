#ifndef DATA_H
#define DATA_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include <pthread.h>

#include <raylib.h>

#include "xrand.h"

typedef enum {
	N_NONE = 0,
	N_SRTD = (1 << 1),
	N_SWPD = (1 << 2),
	N_CMPRD = (1 << 3),
	N_PSRTD = (1 << 4)
} NCol;

typedef struct {
	size_t val;
	NCol col;
} Node;

typedef struct {
	size_t n;
	Node** nodes;
	pthread_mutex_t mutex;
} NArray;

void ninit(Node** node, size_t val, NCol col) {
	*node = (Node*)malloc(sizeof(Node));
	if (*node == NULL) {
		return;
	}
	
	(*node)->col = col;
	(*node)->val = val;
}

void narrinit(NArray** na, size_t n) {
	*na = (NArray*)malloc(sizeof(NArray));
	if (*na == NULL) {
		return;
	}
	(*na)->n = n;
	(*na)->nodes = (Node**)malloc(sizeof(Node*) * n);
	pthread_mutex_init(&(*na)->mutex, NULL);
	if ((*na)->nodes == NULL) {
		return;
	}

	for (size_t i = 0; i < n; i++) {
		ninit(&((*na)->nodes[i]), i + 1, N_NONE);
	}
}

void narrfree(NArray** na) {
    if (na == NULL || *na == NULL) return;
	pthread_mutex_destroy(&(*na)->mutex);
    NArray* arr = *na;
	for (size_t i = 0; i < arr->n; i++) free(arr->nodes[i]);
	free(arr->nodes);
    free(arr);
    *na = NULL;
}

void narrdraw(NArray* na, size_t x, size_t y, size_t w, size_t h) {
	if (na == NULL || na->nodes == NULL) {
		return;
	}

	// pthread_mutex_lock(&na->mutex);

	size_t n = na->n;
	float nw = (float) (w - x) / (float) n;

	for (size_t i = 0; i < n; i++) {
		if (na->nodes[i] == NULL) {
			continue;
		}

		float bh = (float) na->nodes[i]->val / (float) n * (float) h * 1.0f;
		
		Color col = WHITE;

		switch (na->nodes[i]->col) {
			case N_NONE:
				col = WHITE; break;
			case N_CMPRD:
				col = YELLOW; break;
			case N_SRTD:
				col = (Color){180, 255, 0, 255}; break;
			case N_SWPD:
				col = RED; break;
			case N_PSRTD:
				col = DARKGRAY; break;
			default:
				break;
		}

		DrawRectangleV((Vector2){ i * nw + x , h - bh}, (Vector2){nw, bh}, col);
	}

	// pthread_mutex_unlock(&na->mutex);
}

void narrdrawrainbow(NArray* na, size_t x, size_t y, size_t w, size_t h) {
	if (na == NULL || na->nodes == NULL) {
		return;
	}

	// pthread_mutex_lock(&na->mutex);

	size_t n = na->n;
	float nw = (float) (w - x) / (float) n;

	for (size_t i = 0; i < n; i++) {
		if (na->nodes[i] == NULL) {
			continue;
		}

		float bh = (float) na->nodes[i]->val / (float) n * (float) h * 1.0f;
		
		Color col;

		float t = (float)(na->nodes[i]->val - 1) / (n - 1);
		float hue = t * 360.f;

		col = WHITE;

		switch (na->nodes[i]->col) {
			case N_NONE:
				col = ColorFromHSV(hue, 1.f, 1.f); break;
			case N_CMPRD:
				col = GRAY; break;
			case N_SRTD:
				col = ColorFromHSV(hue, 1.f, .5f); break;
			case N_SWPD:
				col = WHITE; break;
			case N_PSRTD:
				col = ColorFromHSV(hue, 1.f, 1.f); break;
			default:
				break;
		}

		DrawRectangleV((Vector2){ i * nw + x , h - bh}, (Vector2){nw, bh}, col);
	}

	// pthread_mutex_unlock(&na->mutex);
}

void narrshuffle(NArray* na, size_t seed) {
	if (na == NULL || na->nodes == NULL) {
		return;
	}

	pthread_mutex_lock(&na->mutex);

	size_t n = na->n;

	init_xorshift_seed(seed);

	for (size_t i = n - 1; i > 0; i--) {
		size_t j = xorshift() % (i + 1);
	
		if (na->nodes[i] == NULL || na->nodes[j] == NULL) {
			return;
		}
		
		Node tmp = *na->nodes[i];
		*na->nodes[i] = *na->nodes[j];
		*na->nodes[j] = tmp;
		na->nodes[i]->col = na->nodes[j]->col = N_NONE;
	}

	pthread_mutex_unlock(&na->mutex);
}

int ncmp(Node* l, Node* r, size_t* cmpcnt, size_t* rdcnt) {
	if (l == NULL || r == NULL) {
		return 0;
	}

	if (cmpcnt != NULL) {
		(*cmpcnt)++;
	}
	if (rdcnt != NULL) {
		(*rdcnt) += 2;
	}

	l->col = N_CMPRD;
	r->col = N_CMPRD;

	if (l->val > r->val) {
		return 1;
	} else if (l->val == r->val) {
		return 0;
	} else {
		return -1;
	}
}

int ncmpnocol(Node* l, Node* r, size_t* cmpcnt, size_t* rdcnt) {
	if (l == NULL || r == NULL) {
		return 0;
	}

	if (cmpcnt != NULL) {
		(*cmpcnt)++;
	}
	if (rdcnt != NULL) {
		(*rdcnt) += 2;
	}

	if (l->val > r->val) {
		return 1;
	} else if (l->val == r->val) {
		return 0;
	} else {
		return -1;
	}
}

void nswap(Node* l, Node* r, size_t* swpcnt, size_t* wrcnt) {
	if (l == NULL || r == NULL) {
		return;
	}

	if (swpcnt != NULL) {
		(*swpcnt)++;
	}
	if (wrcnt != NULL) {
		(*wrcnt) += 3;
	}

	Node tmp = *l;
	*l = *r;
	*r = tmp;

	l->col = N_SWPD;
	r->col = N_SWPD;
}

void narrresize(NArray** na, size_t n) {
	if (*na == NULL) {
		return;
	}
	
	narrfree(na);
	narrinit(na, n);
}

#endif // DATA_H

