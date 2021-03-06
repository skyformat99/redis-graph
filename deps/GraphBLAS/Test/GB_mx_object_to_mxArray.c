//------------------------------------------------------------------------------
// GB_mx_object_to_mxArray
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017, All Rights Reserved.
// http://suitesparse.com   See GraphBLAS/Doc/License.txt for license.

//------------------------------------------------------------------------------

// Convert a GraphBLAS sparse matrix to a MATLAB struct C containing
// C.matrix and a string C.class.  The GraphBLAS matrix is destroyed.

// This could be done using only user-callable GraphBLAS functions, by
// extracting the tuples and converting them into a MATLAB sparse matrix.  But
// that would be much slower and take more memory.  Instead, most of the work
// can be done by pointers, and directly accessing the internal contents of C.
// If C has type GB_BOOL_code or GB_FP64_code, then C can be converted to a
// MATLAB matrix in constant time with essentially no extra memory allocated.
// This is faster, but it means that this MATLAB interface will only work with
// this specific implementation of GraphBLAS.

// Note that the GraphBLAS matrix may contain explicit zeros.  These entries
// should not appear in a MATLAB matrix but MATLAB handles them without
// difficulty.  They are returned to MATLAB in C.matrix.  If any work is done
// in MATLAB on the matrix, these entries will get dropped.  If they are to be
// preserved, do C.pattern = spones (C.matrix) in MATLAB before modifying
// C.matrix.

#include "GB_mex.h"

static const char *MatrixFields [ ] = { "matrix", "class", "values" } ;

mxArray *GB_mx_object_to_mxArray   // returns the MATLAB mxArray
(
    GrB_Matrix *handle,             // handle of GraphBLAS matrix to convert
    const char *name,
    const bool create_struct        // if true, then return a struct
)
{

    // get the inputs
    mxArray *A, *Aclass, *Astruct, *X = NULL ;
    GrB_Matrix C = *handle ;

    ASSERT_OK (GB_check (C, name, 0)) ;

    // make sure there are no pending computations
    GB_wait (C) ;

    ASSERT_OK (GB_check (C, "after assembling pending tuples", 0)) ;

    ASSERT (!C->i_shallow && !C->x_shallow && !C->p_shallow) ;
    int64_t snz = NNZ (C) ;
    mxClassID C_classID = GB_mx_Type_to_classID (C->type) ;

    // MATLAB doesn't want NULL pointers in its empty matrices
    if (C->x == NULL)
    {
        ASSERT (C->nzmax == 0 && snz == 0) ;
        GB_CALLOC_MEMORY (C->x, 1, sizeof (double)) ;
        C->x_shallow = false ;
    }
    if (C->i == NULL)
    {
        ASSERT (C->nzmax == 0 && snz == 0) ;
        GB_CALLOC_MEMORY (C->i, 1, sizeof (int64_t)) ;
        C->i_shallow = false ;
    }
    if (C->p == NULL)
    {
        ASSERT (C->nzmax == 0 && snz == 0) ;
        GB_CALLOC_MEMORY (C->p, C->ncols + 1, sizeof (int64_t)) ;
        C->i_shallow = false ;
    }

    C->nzmax = IMAX (C->nzmax, 1) ;

    // create the MATLAB matrix A and link in the numerical values of C
    if (C->type->code == GB_BOOL_code)
    {
        // C is boolean, which is the same as a MATLAB logical sparse matrix
        A = mxCreateSparseLogicalMatrix (0, 0, 0) ;
        mexMakeMemoryPersistent (C->x) ;
        mxSetData (A, (bool *) C->x) ;
        C->x_shallow = false ;

        // C->x is treated as if it was freed
        AS_IF_FREE (C->x) ;   // unlink C->x from C since it's now in MATLAB C

    }
    else if (C->type->code == GB_FP64_code)
    {
        // C is double, which is the same as a MATLAB double sparse matrix
        A = mxCreateSparse (0, 0, 0, mxREAL) ;
        mexMakeMemoryPersistent (C->x) ;
        mxSetData (A, C->x) ;
        C->x_shallow = false ;

        // C->x is treated as if it was freed
        AS_IF_FREE (C->x) ;   // unlink C->x from C since it's now in MATLAB C

    }
    else if (C->type == Complex)
    {

        // user-defined Complex type
        A = mxCreateSparse (C->nrows, C->ncols, C->nzmax, mxCOMPLEX) ;
        GB_mx_complex_split (snz, C->x, A) ;

    }
    else
    {
        // otherwise C is cast into a MATLAB double sparse matrix
        A = mxCreateSparse (0, 0, 0, mxREAL) ;
        GB_MALLOC_MEMORY (double *Sx, snz+1, sizeof (double)) ;
        GB_cast_array (Sx, GB_FP64_code, C->x, C->type->code, snz) ;
        mexMakeMemoryPersistent (Sx) ;
        mxSetPr (A, Sx) ;

        // Sx was just malloc'd, and given to MATLAB.  Treat it as if
        // GraphBLAS has freed it
        AS_IF_FREE (Sx) ;

        if (create_struct)
        {
            // If C is int64 or uint64, then typecasting can lose information,
            // so keep an uncasted copy of C->x as well.
            X = mxCreateNumericMatrix (0, 0, C_classID, mxREAL) ;
            mxSetM (X, snz) ;
            mxSetN (X, 1) ;
            mxSetData (X, C->x) ;
            mexMakeMemoryPersistent (C->x) ;
            C->x_shallow = false ;
            // treat C->x as if it were freed
            AS_IF_FREE (C->x) ;
        }
    }

    // set nrows, ncols, nzmax, and the pattern of A
    mxSetM (A, C->nrows) ;
    mxSetN (A, C->ncols) ;
    mxSetNzmax (A, C->nzmax) ;
    mxFree (mxGetJc (A)) ;
    mxFree (mxGetIr (A)) ;
    mexMakeMemoryPersistent (C->p) ;
    mexMakeMemoryPersistent (C->i) ;
    mxSetJc (A, (size_t *) C->p) ;
    mxSetIr (A, (size_t *) C->i) ;
    C->p_shallow = true ;
    C->i_shallow = true ;

    // treat C->p as if freed
    AS_IF_FREE (C->p) ;

    // treat C->i as if freed
    AS_IF_FREE (C->i) ;

    // free C, but leave any shallow components untouched
    // since these have been transplanted into the MATLAB matrix.
    GB_MATRIX_FREE (handle) ;

    if (create_struct)
    {
        // create the class
        Aclass = GB_mx_classID_to_string (C_classID) ;
        // create the output struct
        Astruct = mxCreateStructMatrix (1, 1,
           (X == NULL) ? 2 : 3, MatrixFields) ;
        mxSetFieldByNumber (Astruct, 0, 0, A) ;
        mxSetFieldByNumber (Astruct, 0, 1, Aclass) ;
        if (X != NULL)
        {
            mxSetFieldByNumber (Astruct, 0, 2, X) ;
        }
        return (Astruct) ;
    }
    else
    {
        return (A) ;
    }
}

