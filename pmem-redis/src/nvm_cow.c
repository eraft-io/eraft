/***************************************************************************/
/*
 * Copyright (c) 2018, Intel Corporation
 * Copyright (c) 2018, Dennis, Wu <dennis.wu@intel.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/***************************************************************************/
#include "nvm_cow.h"

/*compare the two address*/
static int dictAddrCompare(void *privdata, const void *addr1, const void * addr2) {
    DICT_NOTUSED(privdata);
    return addr1==addr2;
}

static void dictAddrDestructor (void *privdata, void *addr) {
    DICT_NOTUSED(privdata);
    zfree(addr);
}

static uint64_t dictaddrHash(const void *addr) {
    return dictGenHashFunction((unsigned char*)&addr, sizeof(void *));
}


static dictType dbforkType = {
    dictaddrHash,                /* hash function */
    NULL,                       /* key dup */
    NULL,                       /* val dup */
    dictAddrCompare,            /* key compare */
    NULL,                       /* key destructor */
    NULL                        /* val destructor */
};

/*DB->dict, COW dict in which the NVM adress are freeed or duplicated then 
after BGSAVE, the object must be freeded.*/
static dictType dbcowType = {
    dictaddrHash,                /* hash function */
    NULL,                       /* key dup */
    NULL,                       /* val dup */
    dictAddrCompare,            /* key compare */
    dictAddrDestructor,         /* key destructor */
    NULL                        /* val destructor */
};

/***************************************************************************/
/*NVM COW APIs*/
/***************************************************************************/
int cow_isnvmaddrindict(dict *dict, void * addr) {
    dictEntry *de;
    de = dictFind(dict,addr);
    if (de) {
        return 1;
    }
    return 0;
}

void * cow_createforknvmdict() {
    return dictCreate(&dbforkType, NULL);
}

void * cow_createcownvmdict() {
    return dictCreate(&dbcowType,NULL);
}

/* Low level key lookup API, not actually called directly from commands
 * implementations that should instead rely on lookupKeyRead(),
 * lookupKeyWrite() and lookupKeyReadWithFlags(). */
void cow_addaddressindict(dict * dict, void *addr) {   
    dictAddRaw(dict, addr, NULL);
}

void cow_remaddressindict(dict *dict, void *addr) {
    dictDelete(dict,addr);
}

