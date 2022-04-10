// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2021, Intel Corporation */

/*
 * pmemkv_open.cpp -- example usage of pmemkv with already existing pools.
 */

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <libpmemkv.hpp>
#include <string>

#define ASSERT(expr)                                                                     \
	do {                                                                             \
		if (!(expr))                                                             \
			std::cout << pmemkv_errormsg() << std::endl;                     \
		assert(expr);                                                            \
	} while (0)
#define LOG(msg) std::cout << msg << std::endl

using namespace pmem::kv;

//! [open]
/*
 * This example expects a path to already created database pool.
 *
 * Normally you want to re-use a pool, which was created
 * by a previous run of pmemkv application. However, for this example
 * you may want to create pool by hand - use one of the following commands.
 *
 * For regular pools use:
 * pmempool create -l -s 1G "pmemkv" obj path_to_a_pool
 *
 * For poolsets use:
 * pmempool create -l "pmemkv" obj ../examples/example.poolset
 *
 * Word of explanation: "pmemkv" is a pool layout used by cmap engine.
 * For other engines, this may vary, hence it's not advised to create pool manually.
 */
int main(int argc, char *argv[])
{
	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " dbpath\n";
		exit(1);
	}
	pmem::kv::config cfg;
	pmem::kv::status s = cfg.put_path(std::string(argv[1]));
	if (s != pmem::kv::status::OK) {
		std::cout << "put_path error" << std::endl;
		return -1;
	}
	cfg.put_size(1024 * 1024 * 1024);
	cfg.put_create_if_missing(true);
	auto engine_ = new pmem::kv::db();
	pmem::kv::db kv;
	s = engine_->open("radix", std::move(cfg));
	if (s != pmem::kv::status::OK) {
		std::cout << "open pmemkv error" << std::endl;
	}

	return 0;
}
//! [open]