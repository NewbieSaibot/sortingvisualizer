#ifndef SORTTHREAD_H
#define SORTTHREAD_H

#include <pthread.h>
#include "sortingalgos.h"

typedef enum {
	SORT_BBL,
	SORT_SHK,
	SORT_GNM,
	SORT_SHL,
	SORT_SEL,
	SORT_QCK,
	SORT_MRG,
	SORT_NMRG,
	SORT_HP,
	SORT_BOGO,
} SortType;

typedef struct {
	struct timespec sleepreq;

    NArray* array;
    SortData* stats;
    void* algo_data;
    
    pthread_t thread;

	SortChecker* sc;
    
    bool running;
    bool complete;
    bool paused;
	
	bool forcenxt;
	
	bool shuffling;

	SortType stype;
} SortThread;

static void* threadfunc(void* arg) {
    SortThread* t = (SortThread*)arg;

    while (t->running && !t->complete) {
        pthread_mutex_lock(&t->array->mutex);
        
        if ((!t->paused && !(t->stats->status & SDS_ENDED)) || (!(t->stats->status & SDS_ENDED) && t->forcenxt)) {
			if (t->shuffling) {
				shuffleiter(t->array, t->stats, (ShuffleData**)&t->algo_data);
                if (t->stats->status & SDS_ENDED) {
					t->stats->status = SDS_NONE;
                    t->shuffling = false;
					if (t->algo_data) {
                        free(t->algo_data);
                        t->algo_data = NULL;
                    }
                }
            } else {
                switch (t->stype) {
                    case SORT_QCK: qsortiter(t->array, t->stats, (QSortData**)&t->algo_data); break;
                    case SORT_BBL: bubblesortiter(t->array, t->stats, (BubbleSortData**)&t->algo_data); break;
                    case SORT_SEL: selsortiter(t->array, t->stats, (SelectionSortData**)&t->algo_data); break;
                    case SORT_MRG: mergesortiter(t->array, t->stats, (MergeSortData**)&t->algo_data); break;
                    case SORT_SHK: shakersortiter(t->array, t->stats, (ShakerSortData**)&t->algo_data); break;
                    case SORT_NMRG: nmergesortiter(t->array, t->stats, (NaturalMergeSortData**)&t->algo_data); break;
                    case SORT_GNM: gnomesortiter(t->array, t->stats, (GnomeSortData**)&t->algo_data); break;
                    case SORT_SHL: shellsortiter(t->array, t->stats, (ShellSortData**)&t->algo_data); break;
                    case SORT_BOGO: bogosortiter(t->array, t->stats, (BogoSortData**)&t->algo_data); break;
                    case SORT_HP: heapsortiter(t->array, t->stats, (HeapSortData**)&t->algo_data); break;
					default: break;
                }
            }

            if (t->stats->status & SDS_ENDED) {
                t->complete = true;
            }
			t->forcenxt = false;
		}
        
        pthread_mutex_unlock(&t->array->mutex);

        nanosleep(&t->sleepreq, NULL);
    }

	if (t->running && t->complete) {
		while (t->sc->run < 2) {
			pthread_mutex_lock(&t->array->mutex);
			sciter(t->array, t->sc);
			pthread_mutex_unlock(&t->array->mutex);

			nanosleep(&t->sleepreq, NULL);
		}
	}
    
    return NULL;
}

void sthreadinit(SortThread* thread, NArray* array, SortData* stats) {
    thread->array = array;
    thread->stats = stats;
	scinit(&thread->sc);
    thread->algo_data = NULL;
    thread->running = false;
    thread->complete = false;
    thread->paused = false;
	thread->forcenxt = false;
	thread->shuffling = true;
    thread->sleepreq.tv_sec = 0;
    thread->sleepreq.tv_nsec = 100000;
	thread->stype = SORT_BBL;
}

void sthreadstart(SortThread* thread) {
    thread->running = true;
    pthread_create(&thread->thread, NULL, threadfunc, thread);
}

void sthreadstop(SortThread* thread) {
    thread->running = false;
    pthread_join(thread->thread, NULL);
}

void sthreadpause(SortThread* thread) {
    pthread_mutex_lock(&thread->array->mutex);
    thread->paused = true;
    pthread_mutex_unlock(&thread->array->mutex);
}

void sthreadresume(SortThread* thread) {
    pthread_mutex_lock(&thread->array->mutex);
    thread->paused = false;
    pthread_mutex_unlock(&thread->array->mutex);
}

void sthreadreset(SortThread* thread) {
    pthread_mutex_lock(&thread->array->mutex);
    thread->running = false;
    pthread_mutex_unlock(&thread->array->mutex);
    pthread_join(thread->thread, NULL);
    
    pthread_mutex_lock(&thread->array->mutex);
    narrshuffle(thread->array, time(NULL));
    sdatareset(thread->stats);
    if (thread->algo_data) {
        free(thread->algo_data);
        thread->algo_data = NULL;
    }
    thread->complete = false;
    thread->paused = false;
    thread->forcenxt = false;
    thread->stats->status = SDS_NONE;
    screset(thread->sc);
    pthread_mutex_unlock(&thread->array->mutex);
    
	thread->shuffling = true;
    thread->running = true;
    pthread_create(&thread->thread, NULL, threadfunc, thread);
}

bool sthreadiscomplete(SortThread* thread) {
    pthread_mutex_lock(&thread->array->mutex);
    bool complete = thread->complete;
    pthread_mutex_unlock(&thread->array->mutex);
    return complete;
}

void sthreadgetstats(SortThread* thread, size_t* comparisons, size_t* swaps,
						size_t* reads, size_t* writes) {
    pthread_mutex_lock(&thread->array->mutex);
    *comparisons = thread->stats->cmpcnt;
    *swaps = thread->stats->swpcnt;
    *reads = thread->stats->rdcnt;
    *writes = thread->stats->wrcnt;
    pthread_mutex_unlock(&thread->array->mutex);
}

void sthreadsetspeed(SortThread* thread, unsigned long long delay_ns) {
    pthread_mutex_lock(&thread->array->mutex);
	thread->sleepreq.tv_sec = delay_ns / 1000000000ll;
	thread->sleepreq.tv_nsec = delay_ns % 1000000000ll;
    pthread_mutex_unlock(&thread->array->mutex);
}

void sthreadforcenxt(SortThread* thread) {
    pthread_mutex_lock(&thread->array->mutex);
	thread->forcenxt = 1;
    pthread_mutex_unlock(&thread->array->mutex);
}

void sthreadchsort(SortThread* thread, SortType st) {
    pthread_mutex_lock(&thread->array->mutex);
    thread->running = false;
    pthread_mutex_unlock(&thread->array->mutex);
    pthread_join(thread->thread, NULL);

    thread->stype = st;
    if (thread->algo_data) {
        free(thread->algo_data);
        thread->algo_data = NULL;
    }
    thread->complete = false;
    thread->paused = false;
    thread->forcenxt = false;
    thread->stats->status = SDS_NONE;
    screset(thread->sc);

    thread->running = true;
    pthread_create(&thread->thread, NULL, threadfunc, thread);
}

void sthreaddestroy(SortThread* thread) {
    sthreadstop(thread);
	scfree(thread->sc);
    if (thread->algo_data) free(thread->algo_data);
}

void sthreadchsz(SortThread* thread, size_t newsz) {
    pthread_mutex_lock(&thread->array->mutex);
    thread->running = false;
    pthread_mutex_unlock(&thread->array->mutex);
    pthread_join(thread->thread, NULL);

    SortType current_stype = thread->stype;

    narrfree(&thread->array);
    NArray* new_array = NULL;
    narrinit(&new_array, newsz);
    narrshuffle(new_array, time(NULL));
    thread->array = new_array;

    if (thread->algo_data) {
        free(thread->algo_data);
        thread->algo_data = NULL;
    }
    scfree(thread->sc);
    scinit(&thread->sc);
    sdatareset(thread->stats);
    thread->complete = false;
    thread->paused = false;
    thread->forcenxt = false;
    thread->stats->status = SDS_NONE;
    thread->stype = current_stype;

    thread->running = true;
    pthread_create(&thread->thread, NULL, threadfunc, thread);
}

void sthreadapply(SortThread* thread, size_t new_size, SortType new_stype) {
    pthread_mutex_lock(&thread->array->mutex);
    thread->running = false;
    pthread_mutex_unlock(&thread->array->mutex);
    pthread_join(thread->thread, NULL);

	if (thread->algo_data) {
        free(thread->algo_data);
        thread->algo_data = NULL;
    }

    if (new_size != thread->array->n) {
        narrfree(&thread->array);
        NArray* new_array = NULL;
        narrinit(&new_array, new_size);
        // narrshuffle(new_array, time(NULL));
        thread->array = new_array;
    } else {
        // narrshuffle(thread->array, time(NULL));
    }

    if (thread->algo_data) {
        free(thread->algo_data);
        thread->algo_data = NULL;
    }

    scfree(thread->sc);
    scinit(&thread->sc);
    sdatareset(thread->stats);
    thread->complete = false;
    thread->paused = false;
    thread->forcenxt = false;
    thread->stats->status = SDS_NONE;
    thread->stype = new_stype;
    thread->running = true;
    thread->shuffling = true;

    pthread_create(&thread->thread, NULL, threadfunc, thread);
}

#endif // SORTTHREAD_H
