/* Name: requests.h
 * Project: usbtoi2c
 * Author: Christian Starkjohann, edited by Michiel van der Coelen
 * Creation Date: 2011-04-1
 * Tabsize: 4
 * Copyright: (c) 2008 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: GNU GPL v2 (see License.txt), GNU GPL v3 or proprietary (CommercialLicense.txt)
 * This Revision: $Id: requests.h 692 2008-11-07 15:07:40Z cs $
 */

/* This header is shared between the firmware and the host software. It
 * defines the USB request numbers (and optionally data types) used to
 * communicate between the host and the device.
 */

#ifndef __REQUESTS_H_INCLUDED__
#define __REQUESTS_H_INCLUDED__

#define CUSTOM_RQ_GET_VERSION 1

#define CUSTOM_RQ_GET_DATA    2

#define CUSTOM_RQ_SET_DATA 3

#define CUSTOM_RQ_RESET 4

#define CUSTOM_RQ_GET_POS_L 5

#define CUSTOM_RQ_GET_POS_H 6

#endif /* __REQUESTS_H_INCLUDED__ */
