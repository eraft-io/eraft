Badge | Status
--- | ---
**Current Version** | [![Current Version](https://img.shields.io/badge/Release-V1.0--RC1-brightgreen.svg)](https://github.com/pmem/pmem-redis)

# What is Redis?

Redis is often referred as a *data structures* server. What this means is that Redis provides access to mutable data structures via a set of commands, which are sent using a *server-client* model with TCP sockets and a simple protocol. So different processes can query and modify the same data structures in a shared way.

Data structures implemented into Redis have a few special properties:

* Redis cares to store them on disk, even if they are always served and modified into the server memory. This means that Redis is fast, but that is also non-volatile.
* Implementation of data structures stress on memory efficiency, so data structures inside Redis will likely use less memory compared to the same data structure modeled using an high level programming language.
* Redis offers a number of features that are natural to find in a database, like replication, tunable levels of durability, cluster, high availability.

Another good example is to think of Redis as a more complex version of memcached, where the operations are not just SETs and GETs, but operations to work with complex data types like Lists, Sets, ordered data structures, and so forth.

If you want to know more, this is a list of selected starting points:

* Introduction to Redis data types. http://redis.io/topics/data-types-intro
* Try Redis directly inside your browser. http://try.redis.io
* The full list of Redis commands. http://redis.io/commands
* There is much more inside the Redis official documentation. http://redis.io/documentation

# What is Pmem-Redis?

[Pmem-Redis](https://github.com/pmem/pmem-redis) is one redis version that support **Intel DCPMM(Data Center Persistent Memory)** based on open source [redis-4.0.0](https://github.com/antirez/redis/tree/4.0). It benefits the redis's performance by taking advantage of DCPMM competitive performance and persistency.  

Basically Pmem-Redis covers many aspects that related to DCPMM usage: 
* Five typical data structures optimization including: String, List, Hash, Set, Zset.
* DCPMM copy-on-write
* Redis LRU for DCPMM
* Redis defragmentation support for DCPMM
* Pointer-based redis AOF
* Persistent ring buffer

# Get code

1. clone the code:
```
git clone https://github.com/pmem/pmem-redis
```
2. PMDK is one submodule of this repo, so you have to init this submodule in order to get code
```
git submodule init
git submodule update
```

# Building Pmem-Redis
## Compile Options
 Switches | Value | Descriptions
-- | -- | --
USE_NVM | yes/no | DCPMM enable Switch. W/O this option, will compile opensource redis that does not support DCPMM.
AEP_COW | yes/no | DCPMM Copy-On-Write Switch. W/O this option, the BG save and replication will not support
SUPPORT_PBA | yes/no | Pointer Based Aof support Switch. W/O this option, PBA is not support, Same AOF mechanism with open source redis.
USE_AOFGUARD | yes/no | Write Turbo with DCPMM option switch. W/O this option, the AOF log write to the SSD by the page cache directly.
 
## How to compile
**Prerequisite**
* Install `autoconf`, `automake`, `libtool`, `numactl-devel` and `tcl` on system

If you want to build the Pmem-Redis for DCPMM, please install following packages as well:
`libndctl-dev`, `libdaxctl-dev` and `libnuma-dev`

**Compile example**:

If you want to build the original opensource redis-4.0.0, run command:

    make

If you want to build the Pmem-Redis for DCPMM, run command:

    make USE_NVM=yes

If you need enable `AEP_COW`, `SUPPORT_PBA` or `USE_AOFGUARD`, you need to enable `USE_NVM` compile option.

# Fixing build problems with dependencies or cached build options

Pmem-Redis has some dependencies which are included into the `deps` directory.
`make` does not automatically rebuild dependencies even if something in
the source code of dependencies changes.

When you update the source code with `git pull` or when code inside the
dependencies tree is modified in any other way, make sure to use the following
command in order to really clean everything and rebuild from scratch:

    make distclean

This will clean: jemalloc, lua, hiredis, linenoise, pmdk, memkind, jemallocat, aofguard.

Also if you force certain build options like 32bit target, no C compiler
optimizations (for debugging purposes), and other similar build time options,
those options are cached indefinitely until you issue a `make distclean`
command.

# Running Pmem-Redis
## New supported server command:
 
 server command | argument | describe
---- | ---- | ----
 --nvm-maxcapacity | Integer (G) | Set the maximum DCPMM capacity of the redis instance
--nvm-dir | Like: /mnt/pmem0 | Path of DCPMM DAX file system
--nvm-threshold | Integer (default 64) | Threshold of value size, if  greater than  threshold, the value   may be stored on the DCPMM
--pointer-based-aof | yes/no | Enable or disable pointer based AOF
--use-aofguard | yes/no | Enable or disable persistent ring buffer 

## Prepare nvm device for redis to run

Create namespace:

    ndctl create-namespace -m fsdax -r <regionid>
    
Then you may see `/dev/pmem<id>` under `/dev` folder, for example, we have `/dev/pmem0` in our machine.

Format the pmem device:

    mkfs.ext4 /dev/pmem0
    
Mount as dax:
    
    mkdir /mnt/pmem0
    mount -o dax /dev/pmem0 /mnt/pmem0
    
## Run pmem-redis:

    src/redis-server --nvm-maxcapacity 1 --nvm-dir /mnt/pmem0 --nvm-threshold 64

This command starts a redis server that uses DCPMM device(/mnt/pmem0) with max capacity 1GB, and it moves the values whose length are greater than 64 into DCPMM.
 
# Hints 
1. In our Unit Test, we default read `/mnt/pmem0` as pmem device and run test, please modify your pmem device folder to `/mnt/pmem0` to meet the requirement.

2. If you enable --pointer-based-aof in redis server, than pmem-redis will generate one `.ag` file under `/mnt/pmem0` device which link to the AOF file generated on disk. If you need to delete the AOF file, please make sure to delete the `.ag` file at the same time to avoid AOF load error.

# Code contributions

1. Please fork this repositry to your github account and start deveopment, please refer to redis code style
2. Pull requests let you tell others about changes you've pushed to a repository on GitHub. 
3. Once a pull request is opened, you can discuss and review the potential changes with collaborators and add follow-up commits before the changes are merged into the repository.

Note: When working with pull requests, keep the following in mind:

If you're working in the shared repository model, we recommend that you use a topic branch for your pull request. While you can send pull requests from any branch or commit, with a topic branch you can push follow-up commits if you need to update your proposed changes.
When pushing commits to a pull request, don't force push. Force pushing can corrupt your pull request.
After initializing a pull request, you'll see a review page that shows a high-level overview of the changes between your branch (the compare branch) and the repository's base branch. You can add a summary of the proposed changes, review the changes made by commits, add labels, milestones, and assignees, and @mention individual contributors or teams. For more information, see "Creating a pull request."
![pull request page](https://help.github.com/assets/images/help/pull_requests/pull-request-review-page.png)

Once you've created a pull request, you can push commits from your topic branch to add them to your existing pull request. These commits will appear in chronological order within your pull request and the changes will be visible in the "Files changed" tab.

Other contributors can review your proposed changes, add review comments, contribute to the pull request discussion, and even add commits to the pull request.

Enjoy!
