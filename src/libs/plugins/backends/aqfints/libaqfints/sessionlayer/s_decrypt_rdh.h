/***************************************************************************
 begin       : Sun Oct 27 2019
 copyright   : (C) 2019 by Martin Preuss
 email       : martin@libchipcard.de

 ***************************************************************************
 * This file is part of the project "AqBanking".                           *
 * Please see toplevel file COPYING of that project for license details.   *
 ***************************************************************************/

#ifndef AQFINTS_SESSION_DECRYPT_RDH_H
#define AQFINTS_SESSION_DECRYPT_RDH_H



#include "sessionlayer/session.h"
#include "parser/segment.h"




int AQFINTS_Session_DecryptSegmentRdh(AQFINTS_SESSION *sess,
                                      AQFINTS_SEGMENT *segCryptHead,
                                      AQFINTS_SEGMENT *segCryptData,
                                      int secProfileVersion,
                                      const AQFINTS_KEYDESCR *keyDescr,
                                      AQFINTS_SEGMENT_LIST *segmentList);


#endif

