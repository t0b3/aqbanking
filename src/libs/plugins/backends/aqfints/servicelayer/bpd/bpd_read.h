/***************************************************************************
 begin       : Sat Augl 03 2019
 copyright   : (C) 2019 by Martin Preuss
 email       : martin@libchipcard.de

 ***************************************************************************
 * This file is part of the project "AqBanking".                           *
 * Please see toplevel file COPYING of that project for license details.   *
 ***************************************************************************/


#ifndef AQFINTS_BPD_READ_H
#define AQFINTS_BPD_READ_H


#include "servicelayer/bpd/bpd.h"
#include "msglayer/parser/parser.h"
#include "msglayer/parser/segment.h"

#include <gwenhywfar/db.h>


AQFINTS_BPD *AQFINTS_Bpd_SampleBpdFromSegmentList(AQFINTS_PARSER *parser,
                                                  AQFINTS_SEGMENT_LIST *segmentList,
                                                  int removeFromSegList);


AQFINTS_BANKDATA *AQFINTS_Bpd_ReadBankData(GWEN_DB_NODE *db);
AQFINTS_BPDADDR *AQFINTS_Bpd_ReadBpdAddr(GWEN_DB_NODE *db);
AQFINTS_BPDADDR_SERVICE *AQFINTS_Bpd_ReadBpdAddrService(GWEN_DB_NODE *db);

AQFINTS_BPDJOB *AQFINTS_Bpd_ReadBpdJob(GWEN_DB_NODE *db);




#endif

