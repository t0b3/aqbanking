/***************************************************************************
 $RCSfile$
 -------------------
 cvs         : $Id$
 begin       : Mon Mar 01 2004
 copyright   : (C) 2004 by Martin Preuss
 email       : martin@libchipcard.de

 ***************************************************************************
 *          Please see toplevel file COPYING for license details           *
 ***************************************************************************/


#ifndef AQBANKING_JOBSINGLEDEBITNOTE_BE_H
#define AQBANKING_JOBSINGLEDEBITNOTE_BE_H


#include <aqbanking/jobsingledebitnote.h>


/** @addtogroup G_AB_JOBS_XFER_DEBIT
 *
 */
/*@{*/

/** @name Backend Functions
 *
 * Functions in this group are only to be called by banking backends.
 */
/*@{*/

/**
 * This function lets the backend specify the limits for some of the fields
 * of a @ref AB_TRANSACTION.
 */
AQBANKING_API
void AB_JobSingleDebitNote_SetFieldLimits(AB_JOB *j,
                                          AB_TRANSACTION_LIMITS *limits);
/*@}*/ 


/*@}*/ /* addtogroup */

#endif

