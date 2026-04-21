#ifndef SORTINGALGOS_H
#define SORTINGALGOS_H

#include <stdlib.h>
#include <time.h>
#include "data.h"
#include "stack.h"

typedef enum {
	SDS_NONE = 0,
	SDS_BEGUN = (1 << 0),
	SDS_ENDED = (1 << 1),
} SortDataStat;

#define CR_MAX_SZ (1 << 4)

typedef struct {
	int idx[CR_MAX_SZ];
} ColResetter;

typedef struct {
	ColResetter cr;

	struct timespec ts_begin;
	struct timespec ts_en;
	struct timespec ts_diff;

	size_t swpcnt;
	size_t cmpcnt;
	size_t rdcnt;
	size_t wrcnt;

	unsigned int status;
} SortData;

void sdatainit(SortData** sdata) {
	*sdata = (SortData*)malloc(sizeof(SortData));
	if (sdata == NULL) {
		return;
	}
	for (size_t i = 0; i < CR_MAX_SZ; i++) {
		(*sdata)->cr.idx[i] = -1;
	}
	(*sdata)->swpcnt = (*sdata)->cmpcnt = (*sdata)->rdcnt = (*sdata)->wrcnt = 0;
	(*sdata)->status = SDS_NONE;
}

void sdatareset(SortData* sdata) {
    if (sdata == NULL) return;
    
    sdata->swpcnt = 0;
    sdata->cmpcnt = 0;
    sdata->rdcnt = 0;
    sdata->wrcnt = 0;
    sdata->status = SDS_NONE;
}

void sdatafree(SortData** sdata) {
    if (sdata == NULL || *sdata == NULL) return;
    free(*sdata);
    *sdata = NULL;
}

#define crreset(sdata, na) \
	do {\
		for (size_t _i = 0; _i < CR_MAX_SZ && sdata->cr.idx[_i] != -1; _i++) { \
			int _j = sdata->cr.idx[_i]; \
			if (_j >= na->n || na->nodes[_j] == NULL) { \
				continue; \
			} else { \
				if (na->nodes[_j]->col != N_SRTD) na->nodes[_j]->col = N_NONE; \
				sdata->cr.idx[_i] = -1; \
			} \
		} \
	} while(0)

#define cradd(sdata, u, v) do \
	for (size_t _k = 0; _k < CR_MAX_SZ - 1; _k++) { \
		if ((sdata)->cr.idx[_k] == -1 && (sdata)->cr.idx[_k + 1] == -1) { \
			(sdata)->cr.idx[_k] = (u); \
			(sdata)->cr.idx[_k + 1] = (v); \
			break; \
		} \
	} while(0)

typedef enum {
    SHUFFLE_IDLE,
    SHUFFLE_STEP,
    SHUFFLE_DONE
} ShufflePhase;

typedef struct {
    size_t i;
    size_t n;
    ShufflePhase phase;
} ShuffleData;

void shuffleiter(NArray* na, SortData* sdata, ShuffleData** shdata) {
    if (na == NULL || na->nodes == NULL || sdata == NULL) return;
    if (sdata->status & SDS_ENDED) return;

    if (*shdata && (*shdata)->n != na->n) {
        free(*shdata);
        *shdata = NULL;
        sdata->status = SDS_NONE;
        for (int i = 0; i < CR_MAX_SZ; i++) {
            sdata->cr.idx[i] = -1;
        }
    }

    if (!(sdata->status & SDS_BEGUN)) {
        sdata->status |= SDS_BEGUN;
        timespec_get(&sdata->ts_begin, TIME_UTC);

        *shdata = (ShuffleData*)malloc(sizeof(ShuffleData));
        if (*shdata == NULL) return;
        ShuffleData* sh = *shdata;
        sh->n = na->n;
        sh->i = na->n - 1;
        sh->phase = SHUFFLE_STEP;
        return;
    }

    if (*shdata == NULL) return;
    ShuffleData* sh = *shdata;
    crreset(sdata, na);

    switch (sh->phase) {
        case SHUFFLE_STEP:
            if (sh->i == 0) {
                sh->phase = SHUFFLE_DONE;
                break;
            }
            size_t j = xs_rand(0, sh->i + 1);
			nswap(na->nodes[sh->i], na->nodes[j], NULL, NULL);
			cradd(sdata, sh->i, j);
            sh->i--;
            break;

        case SHUFFLE_DONE:
			crreset(sdata, na);
            sdata->status |= SDS_ENDED;
            timespec_get(&sdata->ts_en, TIME_UTC);
            sdata->ts_diff.tv_sec = sdata->ts_en.tv_sec - sdata->ts_begin.tv_sec;
            sdata->ts_diff.tv_nsec = sdata->ts_en.tv_nsec - sdata->ts_begin.tv_nsec;
            if (sdata->ts_diff.tv_nsec < 0) {
                sdata->ts_diff.tv_sec--;
                sdata->ts_diff.tv_nsec += 1000000000;
            }
            free(sh);
            *shdata = NULL;
            break;
        default:
            break;
    }
}

typedef struct {
	size_t i;
	size_t n;
	size_t nn;
	int c;
	bool jstcmpd;
} BubbleSortData;

void bubblesortiter(NArray* na, SortData* sdata, BubbleSortData** bsdata) {
	if (na == NULL || na->nodes == NULL || sdata == NULL) {
		return;
	}

	if (sdata->status & SDS_ENDED) {
		return;
	}
	
	if (!(sdata->status & SDS_BEGUN)) {
		sdata->status |= SDS_BEGUN;
		timespec_get(&sdata->ts_begin, TIME_UTC);
		
		*bsdata = (BubbleSortData*)malloc(sizeof(BubbleSortData));
		if (*bsdata == NULL) {
			return;
		}
		(*bsdata)->i = 1;
		(*bsdata)->n = na->n;
		(*bsdata)->nn = 0;
		(*bsdata)->c = 0;
		(*bsdata)->jstcmpd = false;
		return;
	}
	
	if (*bsdata == NULL) {
		return;
	}

	crreset(sdata, na);

	if ((*bsdata)->i < (*bsdata)->n) {
		if (!(*bsdata)->jstcmpd) {
			(*bsdata)->c = ncmp(na->nodes[(*bsdata)->i - 1], na->nodes[(*bsdata)->i], 
						 &sdata->cmpcnt, &sdata->rdcnt);

			cradd(sdata, (*bsdata)->i - 1, (*bsdata)->i);
		} else {
			if ((*bsdata)->c > 0) {
				nswap(na->nodes[(*bsdata)->i - 1], na->nodes[(*bsdata)->i], 
					  &sdata->swpcnt, &sdata->wrcnt);
				(*bsdata)->nn = (*bsdata)->i;
				cradd(sdata, (*bsdata)->i - 1, (*bsdata)->i);
			}
			
			(*bsdata)->i++;
		}
		(*bsdata)->jstcmpd ^= 1;
	} else {
		if ((*bsdata)->nn == 0) {
			sdata->status |= SDS_ENDED;
			timespec_get(&sdata->ts_en, TIME_UTC);
			
			sdata->ts_diff.tv_sec = sdata->ts_en.tv_sec - sdata->ts_begin.tv_sec;
			sdata->ts_diff.tv_nsec = sdata->ts_en.tv_nsec - sdata->ts_begin.tv_nsec;
			if (sdata->ts_diff.tv_nsec < 0) {
				sdata->ts_diff.tv_sec--;
				sdata->ts_diff.tv_nsec += 1000000000;
			}
			
			free(*bsdata);
			*bsdata = NULL;
		} else {
			for (size_t j = (*bsdata)->nn; j < (*bsdata)->n; j++) {
				na->nodes[j]->col = N_SRTD;
			}
			(*bsdata)->n = (*bsdata)->nn;
			(*bsdata)->i = 1;
			(*bsdata)->nn = 0;
		}
	}
}

typedef struct {
	size_t i;
	size_t rt;
	size_t lt;
	size_t nrt;
	size_t nlt;
	int c;
	bool rev;
	bool jstcmpd;
} ShakerSortData;

void shakersortiter(NArray* na, SortData* sdata, ShakerSortData** shsdata) {
    if (na == NULL || na->nodes == NULL || sdata == NULL) {
        return;
    }

    if (sdata->status & SDS_ENDED) {
        return;
    }
    
    if (!(sdata->status & SDS_BEGUN)) {
        sdata->status |= SDS_BEGUN;
        timespec_get(&sdata->ts_begin, TIME_UTC);
        
        *shsdata = (ShakerSortData*)malloc(sizeof(ShakerSortData));
        if (*shsdata == NULL) {
            return;
        }
        (*shsdata)->i = 1;
        (*shsdata)->lt = 0;
        (*shsdata)->rt = na->n;
        (*shsdata)->nlt = 0;
        (*shsdata)->nrt = na->n;
        (*shsdata)->c = 0;
        (*shsdata)->rev = false;
        (*shsdata)->jstcmpd = false;
        return;
    }
    
    if (*shsdata == NULL) {
        return;
    }

    crreset(sdata, na);
    
    if ((*shsdata)->lt < (*shsdata)->rt) {
        if (!(*shsdata)->jstcmpd) {
            if ((*shsdata)->lt < (*shsdata)->i && (*shsdata)->i < (*shsdata)->rt) {
                if (!(*shsdata)->rev) {
                    (*shsdata)->c = ncmp(na->nodes[(*shsdata)->i - 1], na->nodes[(*shsdata)->i], 
                                 &sdata->cmpcnt, &sdata->rdcnt);
					for (size_t j = 0; j < CR_MAX_SZ - 1; j++) {
						if (sdata->cr.idx[j] == -1 && sdata->cr.idx[j + 1] == -1) {
							sdata->cr.idx[j] = (*shsdata)->i - 1;
							sdata->cr.idx[j + 1] = (*shsdata)->i;
							break;
						}
					}
                } else {
                    (*shsdata)->c = ncmp(na->nodes[(*shsdata)->i - 1], na->nodes[(*shsdata)->i], 
                                 &sdata->cmpcnt, &sdata->rdcnt);
					cradd(sdata, (*shsdata)->i - 1, (*shsdata)->i);
                }
                (*shsdata)->jstcmpd = 1;
            } else {
                if (!(*shsdata)->rev) {
                    size_t old_rt = (*shsdata)->rt;
                    for (size_t j = (*shsdata)->nrt; j < (*shsdata)->rt; j++) {
                        na->nodes[j]->col = N_SRTD;
                    }
                    (*shsdata)->rt = (*shsdata)->nrt;
                    if ((*shsdata)->rt == old_rt) {
                        (*shsdata)->lt = (*shsdata)->rt;
                    } else {
                        (*shsdata)->i = (*shsdata)->rt > 0 ? (*shsdata)->rt - 1 : 0;
                    }
                } else {
                    size_t old_lt = (*shsdata)->lt;
                    if ((*shsdata)->nlt >= (*shsdata)->lt) {
                        for (size_t j = (*shsdata)->nlt; j >= (*shsdata)->lt; j--) {
                            na->nodes[j]->col = N_SRTD;
                            if (j == 0) break;
                        }
                    }
                    (*shsdata)->lt = (*shsdata)->nlt;
                    if ((*shsdata)->lt == old_lt) {
                        (*shsdata)->rt = (*shsdata)->lt;
                    } else {
                        (*shsdata)->i = (*shsdata)->lt + 1;
                    }
                }
                (*shsdata)->rev ^= 1;
                (*shsdata)->jstcmpd = 0;
                if ((*shsdata)->lt + 1 >= (*shsdata)->rt) {
                    (*shsdata)->lt = (*shsdata)->rt;
                }
            }
        } else {
            if ((*shsdata)->lt < (*shsdata)->i && (*shsdata)->i < (*shsdata)->rt) {
                if ((*shsdata)->c > 0) {
                    nswap(na->nodes[(*shsdata)->i - 1], na->nodes[(*shsdata)->i], 
                          &sdata->swpcnt, &sdata->wrcnt);
					cradd(sdata, (*shsdata)->i - 1, (*shsdata)->i);
                    if (!(*shsdata)->rev) {
                        (*shsdata)->nrt = (*shsdata)->i;
                    } else {
                        (*shsdata)->nlt = (*shsdata)->i;
                    }
                }
                if (!(*shsdata)->rev)
                    (*shsdata)->i++;
                else if ((*shsdata)->i > 0)
                    (*shsdata)->i--;
                (*shsdata)->jstcmpd = 0;
            } else {
                (*shsdata)->jstcmpd = 0;
            }
        }
    } else {
        sdata->status |= SDS_ENDED;
        timespec_get(&sdata->ts_en, TIME_UTC);
        sdata->ts_diff.tv_sec = sdata->ts_en.tv_sec - sdata->ts_begin.tv_sec;
        sdata->ts_diff.tv_nsec = sdata->ts_en.tv_nsec - sdata->ts_begin.tv_nsec;
        if (sdata->ts_diff.tv_nsec < 0) {
            sdata->ts_diff.tv_sec--;
            sdata->ts_diff.tv_nsec += 1000000000;
        }
        free(*shsdata);
        *shsdata = NULL;
    }
}

typedef enum {
    GS_IDLE,
    GS_SRTNG,
    GS_DONE
} GnomeSortPhase;

typedef struct {
    size_t i;
    size_t n;
    GnomeSortPhase phase;
} GnomeSortData;

void gnomesortiter(NArray* na, SortData* sdata, GnomeSortData** gsdata) {
    if (na == NULL || na->nodes == NULL || sdata == NULL) return;
    if (sdata->status & SDS_ENDED) return;

    if (*gsdata && (*gsdata)->n != na->n) {
        free(*gsdata);
        *gsdata = NULL;
        sdata->status = SDS_NONE;
    }

    if (!(sdata->status & SDS_BEGUN)) {
        sdata->status |= SDS_BEGUN;
        timespec_get(&sdata->ts_begin, TIME_UTC);

        *gsdata = (GnomeSortData*)malloc(sizeof(GnomeSortData));
        if (*gsdata == NULL) return;
        (*gsdata)->i = 1;
        (*gsdata)->n = na->n;
        (*gsdata)->phase = GS_SRTNG;
        return;
    }

    if (*gsdata == NULL) return;
    GnomeSortData* gs = *gsdata;
    crreset(sdata, na);

    if (gs->phase == GS_DONE) {
        sdata->status |= SDS_ENDED;
        timespec_get(&sdata->ts_en, TIME_UTC);
        sdata->ts_diff.tv_sec = sdata->ts_en.tv_sec - sdata->ts_begin.tv_sec;
        sdata->ts_diff.tv_nsec = sdata->ts_en.tv_nsec - sdata->ts_begin.tv_nsec;
        if (sdata->ts_diff.tv_nsec < 0) {
            sdata->ts_diff.tv_sec--;
            sdata->ts_diff.tv_nsec += 1000000000;
        }
        free(gs);
        *gsdata = NULL;
        return;
    }

    if (gs->i < gs->n) {
        int cmp = ncmp(na->nodes[gs->i - 1], na->nodes[gs->i],
                       &sdata->cmpcnt, &sdata->rdcnt);
        cradd(sdata, gs->i - 1, gs->i);
        if (cmp > 0) {
            nswap(na->nodes[gs->i - 1], na->nodes[gs->i],
                  &sdata->swpcnt, &sdata->wrcnt);
            cradd(sdata, gs->i - 1, gs->i);
            if (gs->i > 1) {
                gs->i--;
            }
        } else {
            gs->i++;
        }
    } else {
        gs->phase = GS_DONE;
        for (size_t idx = 0; idx < gs->n; idx++) {
            na->nodes[idx]->col = N_SRTD;
        }
    }
}

typedef enum {
    SS_IDLE,
    SS_GAPNXT,
    SS_INSSTART,
    SS_INSSTEP,
    SS_DONE
} ShellSortPhase;

typedef struct {
    size_t gap;
    size_t i;
    size_t j;
    size_t n;
    ShellSortPhase phase;
} ShellSortData;

void shellsortiter(NArray* na, SortData* sdata, ShellSortData** ssdata) {
    if (na == NULL || na->nodes == NULL || sdata == NULL) return;
    if (sdata->status & SDS_ENDED) return;

    if (*ssdata && (*ssdata)->n != na->n) {
        free(*ssdata);
        *ssdata = NULL;
        sdata->status = SDS_NONE;
        for (int i = 0; i < CR_MAX_SZ; i++) {
            sdata->cr.idx[i] = -1;
        }
    }

    if (!(sdata->status & SDS_BEGUN)) {
        sdata->status |= SDS_BEGUN;
        timespec_get(&sdata->ts_begin, TIME_UTC);

        *ssdata = (ShellSortData*)malloc(sizeof(ShellSortData));
        if (*ssdata == NULL) return;
        ShellSortData* ss = *ssdata;
        ss->n = na->n;
        ss->gap = na->n / 2;
        ss->phase = SS_GAPNXT;
        return;
    }

    if (*ssdata == NULL) return;
    ShellSortData* ss = *ssdata;
    crreset(sdata, na);

    switch (ss->phase) {
        case SS_GAPNXT:
            if (ss->gap == 0) {
                ss->phase = SS_DONE;
                for (size_t idx = 0; idx < ss->n; idx++) {
                    na->nodes[idx]->col = N_SRTD;
				}
                break;
            }
            ss->i = ss->gap;
            ss->phase = SS_INSSTART;
            break;

        case SS_INSSTART:
            if (ss->i >= ss->n) {
                ss->gap /= 2;
                ss->phase = SS_GAPNXT;
                break;
            }
            ss->j = ss->i;
            ss->phase = SS_INSSTEP;
            break;

        case SS_INSSTEP:
            if (ss->j < ss->gap) {
                ss->i++;
                ss->phase = SS_INSSTART;
                break;
            }
            size_t l = ss->j - ss->gap;
            size_t r = ss->j;
            int cmp = ncmp(na->nodes[l], na->nodes[r],
                           &sdata->cmpcnt, &sdata->rdcnt);
            cradd(sdata, l, r);
            if (cmp > 0) {
                nswap(na->nodes[l], na->nodes[r],
                      &sdata->swpcnt, &sdata->wrcnt);
                cradd(sdata, l, r);
                ss->j -= ss->gap;
                break;
            } else {
                ss->i++;
                ss->phase = SS_INSSTART;
                break;
            }

        case SS_DONE:
            sdata->status |= SDS_ENDED;
            timespec_get(&sdata->ts_en, TIME_UTC);
            sdata->ts_diff.tv_sec = sdata->ts_en.tv_sec - sdata->ts_begin.tv_sec;
            sdata->ts_diff.tv_nsec = sdata->ts_en.tv_nsec - sdata->ts_begin.tv_nsec;
            if (sdata->ts_diff.tv_nsec < 0) {
                sdata->ts_diff.tv_sec--;
                sdata->ts_diff.tv_nsec += 1000000000;
            }
            free(ss);
            *ssdata = NULL;
            break;
        default:
            break;
    }
}

typedef struct {
	size_t i;
	size_t i_min;
	size_t lt;
	size_t n;
	int c;
	bool jstswpd;
} SelectionSortData;

void selsortiter(NArray* na, SortData* sdata, SelectionSortData** seldata) {
	if (na == NULL || na->nodes == NULL || sdata == NULL) {
		return;
	}

	if (sdata->status & SDS_ENDED) {
		return;
	}
	
	if (!(sdata->status & SDS_BEGUN)) {
		sdata->status |= SDS_BEGUN;
		timespec_get(&sdata->ts_begin, TIME_UTC);
		
		*seldata = (SelectionSortData*)malloc(sizeof(BubbleSortData));
		if (*seldata == NULL) {
			return;
		}
		(*seldata)->i = 1;
		(*seldata)->i_min = 0;
		(*seldata)->lt = 0;
		(*seldata)->n = na->n;
		(*seldata)->c = 0;
		(*seldata)->jstswpd = false;
		return;
	}
	
	if (*seldata == NULL) {
		return;
	}

	crreset(sdata, na);
	
	if ((*seldata)->lt < (*seldata)->n) {
		if (!(*seldata)->jstswpd) {
			if ((*seldata)->i < (*seldata)->n) {
				(*seldata)->c = ncmp(na->nodes[(*seldata)->i], na->nodes[(*seldata)->i_min],
							&sdata->cmpcnt, &sdata->rdcnt);
				cradd(sdata, (*seldata)->i, (*seldata)->i_min);
				if ((*seldata)->c < 0) {
					(*seldata)->i_min = (*seldata)->i;
					sdata->wrcnt++;
				}
				(*seldata)->i++;
			} else {
				(*seldata)->jstswpd = 1;
			}
		} else {
			nswap(na->nodes[(*seldata)->lt], na->nodes[(*seldata)->i_min],
				&sdata->swpcnt, &sdata->wrcnt);
			cradd(sdata, (*seldata)->lt, (*seldata)->i_min);
			na->nodes[(*seldata)->lt]->col = N_SRTD;
			(*seldata)->lt++;
			(*seldata)->i = (*seldata)->lt + 1;
			(*seldata)->i_min = (*seldata)->lt;
			(*seldata)->jstswpd = 0;
		}
	} else {
		sdata->status |= SDS_ENDED;
		timespec_get(&sdata->ts_en, TIME_UTC);
		
		sdata->ts_diff.tv_sec = sdata->ts_en.tv_sec - sdata->ts_begin.tv_sec;
		sdata->ts_diff.tv_nsec = sdata->ts_en.tv_nsec - sdata->ts_begin.tv_nsec;
		if (sdata->ts_diff.tv_nsec < 0) {
			sdata->ts_diff.tv_sec--;
			sdata->ts_diff.tv_nsec += 1000000000;
		}
		
		free(*seldata);
		*seldata = NULL;
	}
}

typedef struct {
	Stack* st;

	size_t l;
	size_t r;
	size_t pvt;
	size_t n;
	int c;
	bool jstcmpd;
} QSortData;

void qsortiter(NArray* na, SortData* sdata, QSortData** qsdata) {
    if (na == NULL || na->nodes == NULL || sdata == NULL) return;
    if (sdata->status & SDS_ENDED) return;

    if (!(sdata->status & SDS_BEGUN)) {
        sdata->status |= SDS_BEGUN;
        timespec_get(&sdata->ts_begin, TIME_UTC);

        *qsdata = (QSortData*)malloc(sizeof(QSortData));
        if (*qsdata == NULL) return;

        (*qsdata)->l = 0;
        (*qsdata)->r = 0;
        (*qsdata)->pvt = 0;
        (*qsdata)->n = 0;
        (*qsdata)->c = 0;
        (*qsdata)->jstcmpd = false;

        stkinit(&(*qsdata)->st, sizeof(size_t));
        if ((*qsdata)->st == NULL) {
            free(*qsdata);
            *qsdata = NULL;
            return;
        }

        size_t l = 0;
        size_t r = na->n;
        stkpush((*qsdata)->st, &l);
        stkpush((*qsdata)->st, &r);
        return;
    }

    if (*qsdata == NULL) return;
    crreset(sdata, na);

    if ((*qsdata)->st->size != 0 || (*qsdata)->jstcmpd) {
        if (!(*qsdata)->jstcmpd) {
            size_t r = *(size_t*)stktop((*qsdata)->st);
            stkpop((*qsdata)->st);
            size_t l = *(size_t*)stktop((*qsdata)->st);
            stkpop((*qsdata)->st);

            if (r - l == 1) {
                na->nodes[l]->col = N_SRTD;
                return;
            }
            if (r - l <= 0) return;

            (*qsdata)->l = l;
            (*qsdata)->r = r;
            (*qsdata)->pvt = l;
            (*qsdata)->n = l;
            (*qsdata)->jstcmpd = true;
        } else {
            size_t l = (*qsdata)->l;
            size_t r = (*qsdata)->r;
            size_t i = (*qsdata)->pvt;
            size_t j = (*qsdata)->n;
            size_t pvtidx = r - 1;

            if (j < pvtidx) {
                (*qsdata)->c = ncmp(na->nodes[j], na->nodes[pvtidx],
                                    &sdata->cmpcnt, &sdata->rdcnt);
                cradd(sdata, j, pvtidx);

                if ((*qsdata)->c <= 0) {
                    nswap(na->nodes[i], na->nodes[j],
                          &sdata->swpcnt, &sdata->wrcnt);
                    cradd(sdata, i, j);
                    i++;
                }
                j++;

                (*qsdata)->pvt = i;
                (*qsdata)->n = j;
            } else {
                nswap(na->nodes[i], na->nodes[pvtidx],
                      &sdata->swpcnt, &sdata->wrcnt);
                cradd(sdata, i, pvtidx);
                na->nodes[i]->col = N_SRTD;

                if (i + 1 < r) {
                    size_t l = i + 1;
                    size_t r = r;
                    stkpush((*qsdata)->st, &l);
                    stkpush((*qsdata)->st, &r);
                }
                if (l < i) {
                    size_t l = l;
                    size_t r = i;
                    stkpush((*qsdata)->st, &l);
                    stkpush((*qsdata)->st, &r);
                }

                (*qsdata)->jstcmpd = false;
            }
        }
    } else {
        sdata->status |= SDS_ENDED;
        timespec_get(&sdata->ts_en, TIME_UTC);

        sdata->ts_diff.tv_sec = sdata->ts_en.tv_sec - sdata->ts_begin.tv_sec;
        sdata->ts_diff.tv_nsec = sdata->ts_en.tv_nsec - sdata->ts_begin.tv_nsec;
        if (sdata->ts_diff.tv_nsec < 0) {
            sdata->ts_diff.tv_sec--;
            sdata->ts_diff.tv_nsec += 1000000000;
        }

        stkfree((*qsdata)->st);
        free(*qsdata);
        *qsdata = NULL;
    }
}

typedef enum {
    MSS_IDLE,
    MSS_CMP,
    MSS_CPYLT,
    MSS_CPYRT,
    MSS_CPYBCK,
    MSS_DONE
} MergeStepPhase;

typedef struct {
    size_t w;
    size_t lt;
    size_t n;

    Node* tmp;
	size_t tmpsz;

    size_t st, mid, en;
    size_t i, j, k;
    MergeStepPhase phase;
} MergeSortData;

void mergesortiter(NArray* na, SortData* sdata, MergeSortData** msdata) {
    if (na == NULL || na->nodes == NULL || sdata == NULL) return;
    if (sdata->status & SDS_ENDED) return;

    if (!(sdata->status & SDS_BEGUN)) {
        sdata->status |= SDS_BEGUN;
        timespec_get(&sdata->ts_begin, TIME_UTC);

        *msdata = (MergeSortData*)malloc(sizeof(MergeSortData));
        if (*msdata == NULL) return;
        MergeSortData* ms = *msdata;
        ms->w = 1;
        ms->lt = 0;
        ms->n = na->n;
        ms->tmp = NULL;
        ms->tmpsz = 0;
        ms->phase = MSS_IDLE;
        return;
    }

    if (*msdata == NULL) return;

    crreset(sdata, na);
    MergeSortData* ms = *msdata;

    if (ms->phase == MSS_DONE) {
        sdata->status |= SDS_ENDED;
        timespec_get(&sdata->ts_en, TIME_UTC);
        sdata->ts_diff.tv_sec  = sdata->ts_en.tv_sec  - sdata->ts_begin.tv_sec;
        sdata->ts_diff.tv_nsec = sdata->ts_en.tv_nsec - sdata->ts_begin.tv_nsec;
        if (sdata->ts_diff.tv_nsec < 0) {
            sdata->ts_diff.tv_sec--;
            sdata->ts_diff.tv_nsec += 1000000000L;
        }
        if (ms->tmp) free(ms->tmp);
        free(*msdata);
        *msdata = NULL;
        return;
    }

    if (ms->phase == MSS_IDLE) {
        if (ms->lt >= ms->n) {
            ms->w *= 2;
            ms->lt = 0;
            if (ms->w >= ms->n) {
                for (size_t i = 0; i < ms->n; i++)
                    na->nodes[i]->col = N_SRTD;
                ms->phase = MSS_DONE;
                return;
            }
        }

        ms->st = ms->lt;
        ms->mid = (ms->st + ms->w < ms->n) ? ms->st + ms->w : ms->n;
        ms->en = (ms->st + 2 * ms->w < ms->n) ? ms->st + 2 * ms->w : ms->n;

        size_t needed = ms->en - ms->st;
        if (needed > ms->tmpsz) {
            if (ms->tmp) free(ms->tmp);
            ms->tmp = (Node*)malloc(needed * sizeof(Node));
            if (!ms->tmp) {
                sdata->status |= SDS_ENDED;
                free(*msdata);
                *msdata = NULL;
                return;
            }
            ms->tmpsz = needed;
        }

        for (size_t idx = ms->st, cnt = 0; idx < ms->en && cnt < CR_MAX_SZ; idx++, cnt++) {
            sdata->cr.idx[cnt] = (int)idx;
            if (na->nodes[idx]->col != N_SRTD)
                na->nodes[idx]->col = N_CMPRD;
        }

        if (ms->mid >= ms->en) {
            for (size_t idx = ms->st; idx < ms->en; idx++)
                na->nodes[idx]->col = N_SRTD;
            ms->lt += 2 * ms->w;
            return;
        }

        ms->i = ms->st;
        ms->j = ms->mid;
        ms->k = 0;
        ms->phase = MSS_CMP;
        return;
    }

    switch (ms->phase) {
        case MSS_CMP:
            if (ms->i < ms->mid && ms->j < ms->en) {
                sdata->cr.idx[0] = (int)ms->i;
                sdata->cr.idx[1] = (int)ms->j;
                for (int t = 2; t < CR_MAX_SZ; t++) sdata->cr.idx[t] = -1;

                int c = ncmp(na->nodes[ms->i], na->nodes[ms->j], &sdata->cmpcnt, &sdata->rdcnt);
                if (c <= 0) {
                    ms->phase = MSS_CPYLT;
                } else {
                    ms->phase = MSS_CPYRT;
                }
                return;
            }
            if (ms->i < ms->mid) { ms->phase = MSS_CPYLT; return; }
            if (ms->j < ms->en)  { ms->phase = MSS_CPYRT; return; }
            ms->phase = MSS_CPYBCK;
            ms->k = 0;
            return;

        case MSS_CPYLT:
            ms->tmp[ms->k] = *na->nodes[ms->i];
            sdata->wrcnt++;
            sdata->cr.idx[0] = (int)ms->i;
            for (int t = 1; t < CR_MAX_SZ; t++) sdata->cr.idx[t] = -1;
            ms->i++;
            ms->k++;
            ms->phase = MSS_CMP;
            return;

        case MSS_CPYRT:
            ms->tmp[ms->k] = *na->nodes[ms->j];
            sdata->wrcnt++;
            sdata->cr.idx[0] = (int)ms->j;
            for (int t = 1; t < CR_MAX_SZ; t++) sdata->cr.idx[t] = -1;
            ms->j++;
            ms->k++;
            ms->phase = MSS_CMP;
            return;

        case MSS_CPYBCK:
            if (ms->k < (ms->en - ms->st)) {
                size_t dest = ms->st + ms->k;
                Node* dest_node = na->nodes[dest];
                dest_node->val = ms->tmp[ms->k].val;
                dest_node->col = ms->tmp[ms->k].col;
                sdata->wrcnt++;
                sdata->cr.idx[0] = (int)dest;
                for (int t = 1; t < CR_MAX_SZ; t++) sdata->cr.idx[t] = -1;
                ms->k++;
                return;
            } else {
                for (size_t idx = ms->st; idx < ms->en; idx++)
                    na->nodes[idx]->col = N_SRTD;
                ms->lt += 2 * ms->w;
                ms->phase = MSS_IDLE;
                return;
            }

        default:
            return;
    }
}

typedef enum {
    NMS_IDLE,
    NMS_SCNRNS,
    NMS_MRGPAIR,
    NMS_MRGCMP,
    NMS_MRGCPYL,
    NMS_MRGCPYR,
    NMS_MRGBCK,
    NMS_DONE
} NaturalMergePhase;

typedef struct {
    size_t* runsts;
    size_t* runlens;
    size_t runcnt;
    size_t runcap;

    size_t* nsts;
    size_t* nlens;
    size_t ncnt;
    size_t ncap;

    size_t pair_idx;
    size_t l;
    size_t r;
    size_t llen;
    size_t rlen;
    size_t i;
    size_t j;
    size_t k;
    Node* tmp;
    size_t tmp_sz;

    size_t n;
    NaturalMergePhase phase;
} NaturalMergeSortData;

void nmergesortiter(NArray* na, SortData* sdata, NaturalMergeSortData** nmsdata) {
    if (na == NULL || na->nodes == NULL || sdata == NULL) return;
    if (sdata->status & SDS_ENDED) return;

    if (!(sdata->status & SDS_BEGUN)) {
        sdata->status |= SDS_BEGUN;
        timespec_get(&sdata->ts_begin, TIME_UTC);

        *nmsdata = (NaturalMergeSortData*)calloc(1, sizeof(NaturalMergeSortData));
        if (*nmsdata == NULL) return;
        NaturalMergeSortData* nms = *nmsdata;
        nms->n = na->n;
        nms->runcap = 16;
        nms->ncap = 16;
        nms->runsts = (size_t*)malloc(nms->runcap * sizeof(size_t));
        nms->runlens = (size_t*)malloc(nms->runcap * sizeof(size_t));
        nms->nsts = (size_t*)malloc(nms->ncap * sizeof(size_t));
        nms->nlens = (size_t*)malloc(nms->ncap * sizeof(size_t));
        if (!nms->runsts || !nms->runlens || !nms->nsts || !nms->nlens) {
            free(nms->runsts);
			free(nms->runlens);
            free(nms->nsts);
			free(nms->nlens);
            free(nms); *nmsdata = NULL;
            return;
        }
        nms->phase = NMS_IDLE;
        return;
    }

    if (*nmsdata == NULL) return;
    NaturalMergeSortData* nms = *nmsdata;
    crreset(sdata, na);

    switch (nms->phase) {
        case NMS_IDLE:
            nms->phase = NMS_SCNRNS;
            nms->runcnt = 0;
            nms->i = 0;
            break;

        case NMS_SCNRNS: {
            if (nms->i >= nms->n) {
                if (nms->runcnt <= 1) {
                    for (size_t idx = 0; idx < nms->n; idx++)
                        na->nodes[idx]->col = N_SRTD;
                    nms->phase = NMS_DONE;
                    break;
                }
                nms->ncnt = 0;
                nms->pair_idx = 0;
                nms->phase = NMS_MRGPAIR;
                break;
            }

            size_t start = nms->i;
            size_t len = 1;
            while (nms->i + 1 < nms->n) {
                int c = ncmp(na->nodes[nms->i], na->nodes[nms->i + 1],
                             &sdata->cmpcnt, &sdata->rdcnt);
                cradd(sdata, nms->i, nms->i + 1);
                if (c <= 0) {
                    len++;
                    nms->i++;
                } else {
                    break;
                }
            }
            if (nms->runcnt >= nms->runcap) {
                nms->runcap *= 2;
                nms->runsts = (size_t*)realloc(nms->runsts, nms->runcap * sizeof(size_t));
                nms->runlens = (size_t*)realloc(nms->runlens, nms->runcap * sizeof(size_t));
            }
            nms->runsts[nms->runcnt] = start;
            nms->runlens[nms->runcnt] = len;
            nms->runcnt++;
            nms->i++;
            break;
        }

        case NMS_MRGPAIR:
            if (nms->pair_idx >= nms->runcnt) {
                free(nms->runsts);
                free(nms->runlens);
                nms->runsts = nms->nsts;
                nms->runlens = nms->nlens;
                nms->runcnt = nms->ncnt;
                nms->runcap = nms->ncap;
                nms->ncap = 16;
                nms->nsts = (size_t*)malloc(nms->ncap * sizeof(size_t));
                nms->nlens = (size_t*)malloc(nms->ncap * sizeof(size_t));
                if (!nms->nsts || !nms->nlens) {
                    nms->phase = NMS_DONE;
                    break;
                }
                nms->ncnt = 0;
                nms->phase = NMS_IDLE;
                break;
            }

            if (nms->pair_idx + 1 >= nms->runcnt) {
                if (nms->ncnt >= nms->ncap) {
                    nms->ncap *= 2;
                    nms->nsts = (size_t*)realloc(nms->nsts, nms->ncap * sizeof(size_t));
                    nms->nlens = (size_t*)realloc(nms->nlens, nms->ncap * sizeof(size_t));
                }
                nms->nsts[nms->ncnt] = nms->runsts[nms->pair_idx];
                nms->nlens[nms->ncnt] = nms->runlens[nms->pair_idx];
                nms->ncnt++;
                nms->pair_idx++;
                break;
            }

            nms->l = nms->runsts[nms->pair_idx];
            nms->llen = nms->runlens[nms->pair_idx];
            nms->r = nms->runsts[nms->pair_idx + 1];
            nms->rlen = nms->runlens[nms->pair_idx + 1];

            size_t total = nms->llen + nms->rlen;
            if (total > nms->tmp_sz) {
                if (nms->tmp) free(nms->tmp);
                nms->tmp = (Node*)malloc(total * sizeof(Node));
                nms->tmp_sz = total;
                if (nms->tmp == NULL) {
                    nms->phase = NMS_DONE;
                    break;
                }
            }

            nms->i = 0;
            nms->j = 0;
            nms->k = 0;
            nms->phase = NMS_MRGCMP;
            break;

        case NMS_MRGCMP:
            if (nms->i < nms->llen && nms->j < nms->rlen) {
                size_t lidx = nms->l + nms->i;
                size_t ridx = nms->r + nms->j;
                int c = ncmpnocol(na->nodes[lidx], na->nodes[ridx],
                             &sdata->cmpcnt, &sdata->rdcnt);
                cradd(sdata, lidx, ridx);
                if (c <= 0) {
                    nms->tmp[nms->k] = *na->nodes[lidx];
                    sdata->wrcnt++;
                    nms->i++;
                } else {
                    nms->tmp[nms->k] = *na->nodes[ridx];
                    sdata->wrcnt++;
                    nms->j++;
                }
                nms->k++;
                break;
            }
            if (nms->i < nms->llen) {
                nms->phase = NMS_MRGCPYL;
            } else if (nms->j < nms->rlen) {
                nms->phase = NMS_MRGCPYR;
            } else {
                nms->phase = NMS_MRGBCK;
                nms->k = 0;
            }
            break;

        case NMS_MRGCPYL:
            if (nms->i < nms->llen) {
                size_t lidx = nms->l + nms->i;
                nms->tmp[nms->k] = *na->nodes[lidx];
                sdata->wrcnt++;
                nms->i++;
                nms->k++;
                break;
            }
            if (nms->j < nms->rlen) {
                nms->phase = NMS_MRGCPYR;
            } else {
                nms->phase = NMS_MRGBCK;
                nms->k = 0;
            }
            break;

        case NMS_MRGCPYR:
            if (nms->j < nms->rlen) {
                size_t ridx = nms->r + nms->j;
                nms->tmp[nms->k] = *na->nodes[ridx];
                sdata->wrcnt++;
                nms->j++;
                nms->k++;
                break;
            }
            nms->phase = NMS_MRGBCK;
            nms->k = 0;
            break;

        case NMS_MRGBCK:
            if (nms->k < nms->llen + nms->rlen) {
                size_t dest = nms->l + nms->k;
                na->nodes[dest]->val = nms->tmp[nms->k].val;
                na->nodes[dest]->col = N_SRTD;
                sdata->wrcnt++;
                cradd(sdata, dest, dest);
                nms->k++;
                break;
            }
            if (nms->ncnt >= nms->ncap) {
                nms->ncap *= 2;
                nms->nsts = (size_t*)realloc(nms->nsts, nms->ncap * sizeof(size_t));
                nms->nlens = (size_t*)realloc(nms->nlens, nms->ncap * sizeof(size_t));
            }
            nms->nsts[nms->ncnt] = nms->l;
            nms->nlens[nms->ncnt] = nms->llen + nms->rlen;
            nms->ncnt++;
            nms->pair_idx += 2;
            nms->phase = NMS_MRGPAIR;
            break;

        case NMS_DONE:
            sdata->status |= SDS_ENDED;
            timespec_get(&sdata->ts_en, TIME_UTC);
            sdata->ts_diff.tv_sec = sdata->ts_en.tv_sec - sdata->ts_begin.tv_sec;
            sdata->ts_diff.tv_nsec = sdata->ts_en.tv_nsec - sdata->ts_begin.tv_nsec;
            if (sdata->ts_diff.tv_nsec < 0) {
                sdata->ts_diff.tv_sec--;
                sdata->ts_diff.tv_nsec += 1000000000;
            }
            if (nms->tmp) free(nms->tmp);
            free(nms->runsts);
            free(nms->runlens);
            free(nms->nsts);
            free(nms->nlens);
            free(nms);
            *nmsdata = NULL;
            break;
    }
}

typedef enum {
    HS_BUILD,
    HS_SIFT,
    HS_EXTRACT,
    HS_DONE
} HeapSortPhase;

typedef struct {
    size_t n;
    size_t heap_sz;
    size_t i;
    size_t root;
    size_t child;
    HeapSortPhase phase;
} HeapSortData;

void heapsortiter(NArray* na, SortData* sdata, HeapSortData** hsdata) {
    if (na == NULL || na->nodes == NULL || sdata == NULL) return;
    if (sdata->status & SDS_ENDED) return;

    if (*hsdata && (*hsdata)->n != na->n) {
        free(*hsdata);
        *hsdata = NULL;
        sdata->status = SDS_NONE;
        for (int i = 0; i < CR_MAX_SZ; i++) sdata->cr.idx[i] = -1;
    }

    if (!(sdata->status & SDS_BEGUN)) {
        sdata->status |= SDS_BEGUN;
        timespec_get(&sdata->ts_begin, TIME_UTC);

        *hsdata = (HeapSortData*)calloc(1, sizeof(HeapSortData));
        if (*hsdata == NULL) return;
        HeapSortData* hs = *hsdata;
        hs->n = na->n;
        hs->heap_sz = na->n;
        hs->i = na->n / 2;
        hs->phase = HS_BUILD;
        return;
    }

    if (*hsdata == NULL) return;
    HeapSortData* hs = *hsdata;
    crreset(sdata, na);

    switch (hs->phase) {
        case HS_BUILD:
            if (hs->i == 0) {
                hs->phase = HS_EXTRACT;
                break;
            }
            hs->root = hs->i - 1;
            hs->phase = HS_SIFT;
            break;

        case HS_SIFT: {
            size_t l = hs->root * 2 + 1;
            size_t r = l + 1;
            size_t mx = hs->root;

            if (l < hs->heap_sz) {
                int cmpl = ncmp(na->nodes[l], na->nodes[mx],
                                    &sdata->cmpcnt, &sdata->rdcnt);
                cradd(sdata, l, mx);
                if (cmpl > 0) mx = l;
            }
            if (r < hs->heap_sz) {
                int cmpr = ncmp(na->nodes[r], na->nodes[mx],
                                     &sdata->cmpcnt, &sdata->rdcnt);
                cradd(sdata, r, mx);
                if (cmpr > 0) mx = r;
            }

            if (mx != hs->root) {
                nswap(na->nodes[hs->root], na->nodes[mx],
                      &sdata->swpcnt, &sdata->wrcnt);
                cradd(sdata, hs->root, mx);
                hs->root = mx;
                break;
            } else {
                if (hs->phase == HS_SIFT) {
                    if (hs->i > 0) {
                        hs->i--;
                        hs->phase = HS_BUILD;
                    } else {
                        hs->phase = HS_EXTRACT;
                    }
                }
                break;
            }
        }

        case HS_EXTRACT:
            if (hs->heap_sz <= 1) {
                hs->phase = HS_DONE;
                for (size_t idx = 0; idx < hs->n; idx++)
                    na->nodes[idx]->col = N_SRTD;
                break;
            }
            size_t root = 0;
            size_t last = hs->heap_sz - 1;
            nswap(na->nodes[root], na->nodes[last],
                  &sdata->swpcnt, &sdata->wrcnt);
            cradd(sdata, root, last);
            na->nodes[last]->col = N_SRTD;
            hs->heap_sz--;
            hs->root = 0;
            hs->phase = HS_SIFT;
            break;

        case HS_DONE:
            sdata->status |= SDS_ENDED;
            timespec_get(&sdata->ts_en, TIME_UTC);
            sdata->ts_diff.tv_sec = sdata->ts_en.tv_sec - sdata->ts_begin.tv_sec;
            sdata->ts_diff.tv_nsec = sdata->ts_en.tv_nsec - sdata->ts_begin.tv_nsec;
            if (sdata->ts_diff.tv_nsec < 0) {
                sdata->ts_diff.tv_sec--;
                sdata->ts_diff.tv_nsec += 1000000000;
            }
            free(hs);
            *hsdata = NULL;
            break;

        default:
            break;
    }
}

typedef enum {
    BS_IDLE,
    BS_CHK,
    BS_SHFFL,
    BS_DONE
} BogoSortPhase;

typedef struct {
    size_t i;
    size_t n;
    BogoSortPhase phase;
} BogoSortData;

void bogosortiter(NArray* na, SortData* sdata, BogoSortData** bsdata) {
    if (na == NULL || na->nodes == NULL || sdata == NULL) return;
    if (sdata->status & SDS_ENDED) return;

    if (*bsdata && (*bsdata)->n != na->n) {
        free(*bsdata);
        *bsdata = NULL;
        sdata->status = SDS_NONE;
        for (int i = 0; i < CR_MAX_SZ; i++) {
            sdata->cr.idx[i] = -1;
        }
    }

    if (!(sdata->status & SDS_BEGUN)) {
        sdata->status |= SDS_BEGUN;
        timespec_get(&sdata->ts_begin, TIME_UTC);

        *bsdata = (BogoSortData*)malloc(sizeof(BogoSortData));
        if (*bsdata == NULL) return;
        BogoSortData* bs = *bsdata;
        bs->n = na->n;
        bs->i = 0;
        bs->phase = BS_CHK;
        return;
    }

    if (*bsdata == NULL) return;
    BogoSortData* bs = *bsdata;
    crreset(sdata, na);

    switch (bs->phase) {
        case BS_CHK:
            if (bs->i + 1 >= bs->n) {
                bs->phase = BS_DONE;
                for (size_t idx = 0; idx < bs->n; idx++) {
                    na->nodes[idx]->col = N_SRTD;
                }
                break;
            }
            int cmp = ncmp(na->nodes[bs->i], na->nodes[bs->i + 1],
                           &sdata->cmpcnt, &sdata->rdcnt);
            cradd(sdata, bs->i, bs->i + 1);
            if (cmp <= 0) {
                bs->i++;
                break;
            } else {
                bs->i = 0;
                bs->phase = BS_SHFFL;
                break;
            }

        case BS_SHFFL:
            if (bs->i + 1 >= bs->n) {
                bs->i = 0;
                bs->phase = BS_CHK;
                break;
            }
            size_t j = bs->i + (rand() % (bs->n - bs->i));
            if (bs->i != j) {
                nswap(na->nodes[bs->i], na->nodes[j],
                      &sdata->swpcnt, &sdata->wrcnt);
                cradd(sdata, bs->i, j);
            }
            bs->i++;
            break;

        case BS_DONE:
            sdata->status |= SDS_ENDED;
            timespec_get(&sdata->ts_en, TIME_UTC);
            sdata->ts_diff.tv_sec = sdata->ts_en.tv_sec - sdata->ts_begin.tv_sec;
            sdata->ts_diff.tv_nsec = sdata->ts_en.tv_nsec - sdata->ts_begin.tv_nsec;
            if (sdata->ts_diff.tv_nsec < 0) {
                sdata->ts_diff.tv_sec--;
                sdata->ts_diff.tv_nsec += 1000000000;
            }
            free(bs);
            *bsdata = NULL;
            break;
        default:
            break;
    }
}

typedef struct {
	size_t i;
	char run;
} SortChecker;

void scinit(SortChecker** sc) {
	*sc = (SortChecker*)malloc(sizeof(SortChecker));
	if (*sc == NULL) {
		return;
	}
	(*sc)->i = 0;
	(*sc)->run = 0;
}

void scfree(SortChecker* sc) {
	if (sc == NULL) {
		return;
	}
	free(sc);
}

void screset(SortChecker* sc) {
	sc->i = sc->run = 0;
}

void sciter(NArray* na, SortChecker* sc) {
	if (sc == NULL || na == NULL || na->nodes == NULL) {
		return;
	}
	
	if (sc->run >= 2) {
		return;
	}

	if (sc->i >= na->n) {
		sc->run++;
		sc->i = 0;
		return;
	}

	if (na->nodes[sc->i] == NULL) {
		return;
	}

	if (sc->run == 0) {
		na->nodes[sc->i++]->col = N_NONE;
	} else {
		na->nodes[sc->i++]->col = N_SRTD;
	}
}

#endif // SORTINGALGOS_H
