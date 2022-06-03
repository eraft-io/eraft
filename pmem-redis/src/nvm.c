/*
 * Copyright (c) 2018, Intel Corporation
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

#include "server.h"
#include <assert.h>
#include <stdlib.h>

#ifdef USE_NVM
#ifdef _DEFAULT_SOURCE
#undef _DEFAULT_SOURCE
#endif
#include "atomicvar.h"
#include "nvm.h"

#define update_nvm_stat_alloc(__n) do { \
    size_t _n = (__n); \
    if (_n&(sizeof(long)-1)) _n += sizeof(long)-(_n&(sizeof(long)-1)); \
    atomicIncr(used_nvm,__n); \
    atomicIncr(alloc_count, 1); \
} while(0)

#define update_nvm_stat_free(__n) do { \
    size_t _n = (__n); \
    if (_n&(sizeof(long)-1)) _n += sizeof(long)-(_n&(sizeof(long)-1)); \
    atomicDecr(used_nvm,__n); \
    atomicDecr(alloc_count, 1); \
} while(0)

static size_t used_nvm = 0;
static size_t alloc_count = 0;

int is_nvm_addr(const void* ptr) {
    if(!server.nvm_base)
        return 0;
    if((const char*)ptr < server.nvm_base)
        return 0;
    if(server.nvm_base + server.nvm_size <= (const char*)ptr)
        return 0;
    return 1;
}

void* nvm_malloc(size_t size) {
#ifdef SUPPORT_PBA
    if(server.pba.loading)
        return NULL;
#endif
    void *ptr = NULL;
    if(server.pmem_kind!=NULL) {
        ptr= memkind_malloc(server.pmem_kind, size);
        /*update_nvm_stat_alloc(memkind_usable_size(server.pmem_kind, ptr));*/
        if (ptr)
            update_nvm_stat_alloc(jemk_malloc_usable_size(ptr));
#ifdef AEP_COW
        if(server.rdb_child_pid != -1) {
            cow_addaddressindict(server.forked_dict, ptr);
        }
#endif
    }
    return ptr;
}

int nvm_free(void* ptr) {
    /*update_nvm_stat_free(memkind_usable_size(server.pmem_kind, ptr));*/
    size_t size = jemk_malloc_usable_size(ptr);
#ifdef AEP_COW
    if(!cow_isnvmaddrindict(server.forked_dict,ptr) && server.rdb_child_pid !=-1) {
        cow_addaddressindict(server.cow_dict, ptr);
        server.cow_nvm_size += size;
        return 1;
    }else if(server.rdb_child_pid != -1) {
        cow_remaddressindict(server.forked_dict, ptr);
    }
#endif
    
    update_nvm_stat_free(size);
    memkind_free(server.pmem_kind, ptr);
    return 1;
}

size_t nvm_usable_size(void* ptr) {
    /*return memkind_usable_size(server.pmem_kind, ptr);*/
    return jemk_malloc_usable_size(ptr);
}

size_t nvm_get_used(void) {
    size_t um;
    atomicGet(used_nvm,um);
    return um;
}

size_t nvm_get_alloc_count(void)
{
    size_t ret;
    atomicGet(alloc_count, ret);
    return ret;
}

size_t nvm_get_rss(void) {
    size_t epoch = 1, resident = 0, sz = sizeof(size_t);
    /* Update the statistics cached by mallctl. */
    jemk_mallctl("epoch", &epoch, &sz, &epoch, sz);
    /* Unlike RSS, this does not include RSS from shared libraries and other non
     * heap mappings. */
    jemk_mallctl("stats.resident", &resident, &sz, NULL, 0);
    return resident;
}


#ifdef HAVE_DEFRAG
void * zmalloc_nvm_no_tcache(size_t size) {
    void *ptr = nvm_malloc(size);
    if(ptr)
        update_nvm_stat_alloc(jemk_malloc_usable_size(ptr));
    return ptr;
}

void zfree_nvm_no_tcache(void *ptr) {
    if (ptr == NULL) return;
    size_t size=jemk_malloc_usable_size(ptr);
    update_nvm_stat_free(size);
    nvm_free(ptr);
}
#endif
#endif
