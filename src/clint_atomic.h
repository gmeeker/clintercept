/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil -*- */
/*
** Clint OpenCL debugging utilities
** Copyright (c) 2012, Digital Anarchy, Inc.
** All rights reserved.
** 
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
** 
** * Redistributions of source code must retain the above copyright notice,
**   this list of conditions and the following disclaimer.
** * Redistributions in binary form must reproduce the above copyright notice,
**   this list of conditions and the following disclaimer in the documentation
**   and/or other materials provided with the distribution.
** 
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
** THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _CLINT_ATOMIC_H_
#define _CLINT_ATOMIC_H_

#if defined(WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <stdint.h>
#include <libkern/OSAtomic.h>
#elif defined(__GNUC__)
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(WIN32)

typedef LONG ClintSpinLock;
typedef LONG ClintAtomicInt;

#define CLINT_SPINLOCK_LOCK(l) { while (InterlockedExchangeAcquire(&(l), 1) != 0) { while (l) {} } }
#define CLINT_SPINLOCK_UNLOCK(l) InterlockedCompareExchangeRelease(&(l), 0, 1)
#define CLINT_ATOMIC_ADD(v, a) (InterlockedExchangeAdd(&(a), v) + v)
#define CLINT_ATOMIC_SUB(v, a) (InterlockedExchangeAdd(&(a), v) - v)

#elif defined(__APPLE__)

typedef OSSpinLock ClintSpinLock;
typedef int32_t ClintAtomicInt;

#define CLINT_SPINLOCK_LOCK(l) OSSpinLockLock(&(l))
#define CLINT_SPINLOCK_UNLOCK(l) OSSpinLockUnlock(&(l))
#define CLINT_ATOMIC_ADD(v, a) OSAtomicAdd32Barrier(v, &(a))
#define CLINT_ATOMIC_SUB(v, a) OSAtomicAdd32Barrier(-v, &(a))

#elif defined(__GNUC__)

typedef int32_t ClintSpinLock;
typedef int32_t ClintAtomicInt;

#define CLINT_SPINLOCK_LOCK(l) { while (__sync_lock_test_and_set(&(l), 1) != 0) { while (l) {} } }
#define CLINT_SPINLOCK_UNLOCK(l) __sync_lock_release(&(l))
#define CLINT_ATOMIC_ADD(v, a) __sync_add_and_fetch(&(a), v)
#define CLINT_ATOMIC_SUB(v, a) __sync_sub_and_fetch(&(a), v)

#endif

#ifdef __cplusplus
}
#endif

#endif // _CLINT_ATOMIC_H_
