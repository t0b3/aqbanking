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


#ifndef AQBANKING_JOBSINGLETRANSFER_P_H
#define AQBANKING_JOBSINGLETRANSFER_P_H


#include <aqbanking/job.h>
#include <aqbanking/transaction.h>
#include "jobsingletransfer_l.h"

typedef struct AB_JOBSINGLETRANSFER AB_JOBSINGLETRANSFER;
struct AB_JOBSINGLETRANSFER {
  AB_TRANSACTION *transaction;
  int maxPurposeLines;
  int *textKeys;
};
void AB_JobSingleTransfer_FreeData(void *bp, void *p);


#endif

