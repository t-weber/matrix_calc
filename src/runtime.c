/**
 * runtime library
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date 17-apr-20
 * @license: see 'LICENSE.GPL' file
 */

#include <stdio.h>
#include <stdlib.h>
#include <float.h>

#include "types.h"


static t_real g_eps = DBL_EPSILON;
static int8_t g_debug = 0;


// ----------------------------------------------------------------------------
// heap management
// ----------------------------------------------------------------------------

// TODO: use an actual heap data structure
struct t_list
{
	struct t_list *next;
	void *elem;
};


struct t_list* lst_append(struct t_list *lst, void *elem)
{
	if(!lst)
		return 0;

	while(lst->next)
		lst = lst->next;

	lst->next = (struct t_list*)calloc(1, sizeof(struct t_list));
	lst->next->elem = elem;
	lst->next->next = 0;

	return lst->next;
}


void lst_remove(struct t_list *lst, void *elem)
{
	struct t_list *lst_prev = 0;

	while(lst)
	{
		// find element
		if(lst->elem == elem)
			break;

		lst_prev = lst;
		lst = lst->next;
	}

	// lst!=0 -> not beyond the list end, i.e. element was found
	// lst_prev!=0 -> not at the list head
	if(lst && lst_prev)
	{
		// remove list element
		lst_prev->next = lst->next;
		free(lst);
	}
}


// list with allocated memory
// (head node is not used!)
static struct t_list lst_mem;


void* ext_heap_alloc(uint64_t num, uint64_t elemsize)
{
	void *mem = calloc(num, elemsize);
	lst_append(&lst_mem, mem);

	if(g_debug)
	{
		printf("%s: count=%ld, elem_size=%ld, mem=%08lx.\n",
			__func__, num, elemsize, (uint64_t)mem);
	}

	return mem;
}


void ext_heap_free(void* mem)
{
	if(!mem)
		return;

	free(mem);
	lst_remove(&lst_mem, mem);

	if(g_debug)
		printf("%s: mem=%08lx.\n", __func__, (uint64_t)mem);
}


void ext_init()
{
	lst_mem.elem = 0;
	lst_mem.next = 0;
}


void ext_deinit()
{
	// look for non-freed memory
	struct t_list *lst = &lst_mem;

	uint64_t leaks = 0;
	while(lst->next)
	{
		lst = lst->next;
		++leaks;
	}

	if(g_debug)
		printf("%s: %ld memory leaks detected.\n", __func__, leaks);
}


void set_debug(t_int dbg)
{
	g_debug = (dbg!=0);
}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// mathematical functions
// ----------------------------------------------------------------------------

/**
 * set float epsilon
 */
void set_eps(t_real eps)
{
	g_eps = eps;
}


/**
 * get float epsilon
 */
t_real get_eps()
{
	return g_eps;
}


/**
 * tests equality of floating point numbers
 */
int ext_equals(t_real x, t_real y, t_real eps)
{
	t_real diff = x-y;
	if(diff < 0.)
		diff = -diff;
	return diff <= eps;
}


/**
 * removes a given row and column of a square matrix
 */
void ext_submat(const t_real* M, t_int N, t_real* M_new, t_int iremove, t_int jremove)
{
	t_int row_new = 0;
	for(t_int row=0; row<N; ++row)
	{
		if(row == iremove)
			continue;

		t_int col_new = 0;
		for(t_int col=0; col<N; ++col)
		{
			if(col == jremove)
				continue;

			M_new[row_new*(N-1) + col_new] = M[row*N + col];
			++col_new;
		}
		++row_new;
	}
}


/**
 * calculates the determinant
 */
t_real ext_determinant(const t_real* M, t_int N)
{
	// special cases
	if(N == 0)
		return 0;
	else if(N == 1)
		return M[0];
	else if(N == 2)
		return M[0*N+0]*M[1*N+1] - M[0*N+1]*M[1*N+0];


	// get row with maximum number of zeros
	t_int row = 0;
	t_int maxNumZeros = 0;
	for(t_int curRow=0; curRow<N; ++curRow)
	{
		t_int numZeros = 0;
		for(t_int curCol=0; curCol<N; ++curCol)
		{
			if(ext_equals(M[curRow*N + curCol], 0, g_eps))
				++numZeros;
		}

		if(numZeros > maxNumZeros)
		{
			row = curRow;
			maxNumZeros = numZeros;
		}
	}


	// recursively expand determiant along a row
	t_real fullDet = 0.;

	t_real *submat = (t_real*)ext_heap_alloc((N-1)*(N-1), sizeof(t_real));
	for(t_int col=0; col<N; ++col)
	{
		const t_real elem = M[row*N + col];
		if(ext_equals(elem, 0, g_eps))
			continue;

		ext_submat(M, N, submat, row, col);
		const t_real sgn = ((row+col) % 2) == 0 ? 1. : -1.;
		fullDet += elem * ext_determinant(submat, N-1) * sgn;
	}
	ext_heap_free(submat);

	return fullDet;
}



/**
 * inverted matrix
 */
t_int ext_inverse(const t_real* M, t_real* I, t_int N)
{
	t_real fullDet = ext_determinant(M, N);

	// fail if determinant is zero
	if(ext_equals(fullDet, 0., g_eps))
		return 0;

	t_real *submat = (t_real*)ext_heap_alloc((N-1)*(N-1), sizeof(t_real));
	for(t_int i=0; i<N; ++i)
	{
		for(t_int j=0; j<N; ++j)
		{
			ext_submat(M, N, submat, i, j);
			const t_real sgn = ((i+j) % 2) == 0 ? 1. : -1.;
			I[j*N + i] = ext_determinant(submat, N-1) * sgn / fullDet;
		}
	}
	ext_heap_free(submat);

	return 1;
}


/**
 * matrix-matrix product: RES^i_j = M1^i_k M2^k_j
 */
void ext_mult(const t_real* M1, const t_real* M2, t_real *RES, t_int I, t_int J, t_int K)
{
	for(t_int i=0; i<I; ++i)
	{
		for(t_int j=0; j<J; ++j)
		{
			RES[i*J + j] = 0.;

			for(t_int k=0; k<K; ++k)
				RES[i*J + j] += M1[i*K + k]*M2[k*J + j];
		}
	}
}


/**
 * matrix power
 */
t_int ext_power(const t_real* M, t_real* P, t_int N, t_int POW)
{
	t_int POW_pos = POW<0 ? -POW : POW;
	t_int status = 1;

	// temporary matrices
	t_real *Mtmp = (t_real*)ext_heap_alloc(N*N, sizeof(t_real));
	t_real *Mtmp2 = (t_real*)ext_heap_alloc(N*N, sizeof(t_real));

	// Mtmp = M
	for(t_int i=0; i<N; ++i)
		for(t_int j=0; j<N; ++j)
			Mtmp[i*N + j] = M[i*N + j];

	// matrix power
	for(t_int i=0; i<POW_pos-1; ++i)
	{
		ext_mult(Mtmp, M, Mtmp2, N, N, N);

		// Mtmp = Mtmp2
		for(t_int i=0; i<N; ++i)
			for(t_int j=0; j<N; ++j)
				Mtmp[i*N + j] = Mtmp2[i*N + j];
	}

	// invert
	if(POW < 0)
		status = ext_inverse(Mtmp, Mtmp2, N);

	// P = Mtmp2
	for(t_int i=0; i<N; ++i)
		for(t_int j=0; j<N; ++j)
			P[i*N + j] = Mtmp2[i*N + j];

	ext_heap_free(Mtmp);
	ext_heap_free(Mtmp2);
	return status;
}



/**
 * transposed matrix
 */
void ext_transpose(const t_real* M, t_real* T, t_int rows, t_int cols)
{
	for(t_int ctr=0; ctr<rows*cols; ++ctr)
	{
		t_int i = ctr/cols;
		t_int j = ctr%cols;
		T[j*rows + i] = M[i*cols + j];
	}
}
// ----------------------------------------------------------------------------
