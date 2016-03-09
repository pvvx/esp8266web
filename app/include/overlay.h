/*
 * overlay.h
 *
 *  Created on: 01/02/2016
 *      Author: PVV
 */
#ifndef _INCLUDE_OVERLAY_H_
#define _INCLUDE_OVERLAY_H_

#include "tcp_srv_conn.h"

typedef int tovl_call(int flg);

extern tovl_call * ovl_call;

int ovl_loader(uint8 *filename);


#endif /* _INCLUDE_OVERLAY_H_ */
