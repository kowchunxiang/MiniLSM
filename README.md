# \# MiniLSM

# 

# MiniLSM is a lightweight \*\*LSM-tree inspired key-value storage engine\*\* written in C++.

# 

# The project demonstrates several fundamental concepts used in modern database systems, including \*\*Write-Ahead Logging (WAL), MemTables, SSTables, tombstones, compaction, crash recovery, thread synchronization, and performance benchmarking\*\*.

# 

# This project is intended for educational purposes and for understanding how persistent key-value databases work internally.

# 

# \---

# 

# \## Features

# 

# MiniLSM currently supports:

# 

# \- `Put(key, value)` — insert or update a key-value pair

# \- `Get(key)` — retrieve a value using a key

# \- `Delete(key)` — logically delete a key using a tombstone

# \- Write-Ahead Log (WAL)

# \- Basic crash recovery

# \- In-memory MemTable

# \- SSTable persistence

# \- Automatic MemTable flushing

# \- Basic SSTable compaction

# \- Tombstone removal during compaction

# \- Thread synchronization using `std::mutex`

# \- Put/Get performance benchmark

# 

# \---

# 

# \## Architecture

# 

# The basic write flow is:

# 

# ```text

# Put(key, value)

# &#x20;      |

# &#x20;      v

# Write-Ahead Log

# &#x20;      |

# &#x20;      v

# &#x20;   MemTable

# &#x20;      |

# &#x20;      | MemTable reaches limit

# &#x20;      v

# &#x20;   SSTable

# &#x20;      |

# &#x20;      | Multiple SSTables

# &#x20;      v

# &#x20;  Compaction

# ```

# 

# The read flow is:

# 

# ```text

# Get(key)

# &#x20;  |

# &#x20;  v

# MemTable

# &#x20;  |

# &#x20;  | Not Found

# &#x20;  v

# Newest SSTable

# &#x20;  |

# &#x20;  v

# Older SSTables

# ```

# 

# \---

# 

# \## How It Works

# 

# \### 1. Write-Ahead Log

# 

# Before a `Put` or `Delete` operation is stored in memory, the operation is written to:

# 

# ```text

# mini\_lsm.wal

# ```

# 

# Example WAL entry for a Put operation:

# 

# ```text

# PUT

# key\_1

# value\_1

# ```

# 

# Example WAL entry for a Delete operation:

# 

# ```text

# DELETE

# key\_1

# ```

# 

# If the program restarts, MiniLSM reads the WAL and replays the recorded operations.

# 

# This provides a simple form of crash recovery.

# 

# \---

# 

# \### 2. MemTable

# 

# Recent data is stored in memory using:

# 

# ```cpp

# std::map<std::string, std::string>

# ```

# 

# The MemTable stores new and updated values before they are written to disk.

# 

# The maximum MemTable size is currently:

# 

# ```text

# 10,000 records

# ```

# 

# When the MemTable reaches this limit, the data is flushed into an SSTable.

# 

# \---

# 

# \### 3. SSTable

# 

# An SSTable is an immutable file used to store key-value data on disk.

# 

# MiniLSM generates files such as:

# 

# ```text

# mini\_lsm\_0.sst

# mini\_lsm\_1.sst

# mini\_lsm\_2.sst

# ...

# ```

# 

# Each entry is stored as:

# 

# ```text

# key

# value

# ```

# 

# After the MemTable is successfully flushed:

# 

# 1\. The SSTable is written to disk

# 2\. The MemTable is cleared

# 3\. The WAL is cleared

# 4\. A new WAL is opened

# 

# \---

# 

# \### 4. Put

# 

# The `Put` operation performs the following steps:

# 

# ```text

# Put(key, value)

# &#x20;     |

# &#x20;     v

# Validate key

# &#x20;     |

# &#x20;     v

# Lock mutex

# &#x20;     |

# &#x20;     v

# Write operation to WAL

# &#x20;     |

# &#x20;     v

# Store value in MemTable

# &#x20;     |

# &#x20;     v

# Check MemTable size

# &#x20;     |

# &#x20;     v

# Flush to SSTable if necessary

# ```

# 

# Example:

# 

# ```cpp

# db.Put("name", "Alice");

# ```

# 

# \---

# 

# \### 5. Get

# 

# The `Get` operation first searches the in-memory MemTable.

# 

# If the key is not found there, MiniLSM searches SSTables from newest to oldest.

# 

# This is important because newer SSTables may contain newer versions of the same key.

# 

# Example:

# 

# ```cpp

# std::string value;

# 

# Status status = db.Get("name", \&value);

# ```

# 

# If the key exists:

# 

# ```text

# value = Alice

# ```

# 

# If the key does not exist, MiniLSM returns:

# 

# ```text

# NotFound

# ```

# 

# \---

# 

# \### 6. Delete

# 

# MiniLSM uses a \*\*tombstone\*\* instead of immediately removing a key.

# 

# The tombstone value is:

# 

# ```text

# \_\_TOMBSTONE\_\_

# ```

# 

# For example:

# 

# ```cpp

# db.Delete("name");

# ```

# 

# Internally:

# 

# ```text

# name -> \_\_TOMBSTONE\_\_

# ```

# 

# When `Get()` finds a tombstone, the key is treated as deleted.

# 

# The tombstone can later be permanently removed during compaction.

# 

# \---

# 

# \## Compaction

# 

# When several SSTables have been created, MiniLSM performs compaction.

# 

# The current implementation starts compaction when the SSTable counter reaches the configured threshold.

# 

# During compaction, MiniLSM:

# 

# \- Reads existing SSTables

# \- Keeps the newest version of each key

# \- Removes older duplicate versions

# \- Removes tombstones

# \- Deletes old SSTable files

# \- Creates a new merged SSTable

# 

# Conceptually:

# 

# ```text

# SSTable 0

# &#x20;   +

# SSTable 1

# &#x20;   +

# SSTable 2

# &#x20;   |

# &#x20;   v

# &#x20;Compaction

# &#x20;   |

# &#x20;   v

# Merged SSTable

# ```

# 

# Example:

# 

# Before compaction:

# 

# ```text

# SSTable 0:

# A -> 10

# B -> 20

# 

# SSTable 1:

# A -> 30

# C -> 40

# 

# SSTable 2:

# B -> \_\_TOMBSTONE\_\_

# ```

# 

# After compaction:

# 

# ```text

# A -> 30

# C -> 40

# ```

# 

# The old value of `A` is removed and the deleted key `B` is permanently removed.

# 

# \---

# 

# \## Thread Safety

# 

# MiniLSM uses:

# 

# ```cpp

# std::mutex

# ```

# 

# and:

# 

# ```cpp

# std::lock\_guard<std::mutex>

# ```

# 

# to protect shared in-memory data.

# 

# This prevents multiple threads from modifying the MemTable at the same time during protected operations.

# 

# \---

# 

# \## Status Handling

# 

# The project contains a simple `Status` class for reporting operation results.

# 

# Current status codes include:

# 

# ```cpp

# kOk

# kNotFound

# kInvalidArgument

# ```

# 

# Examples:

# 

# ```cpp

# Status::OK();

# Status::NotFound();

# Status::InvalidArgument("key is empty");

# ```

# 

# This provides a cleaner way to report database operation results.

# 

# \---

# 

# \## Crash Recovery

# 

# When MiniLSM starts, it performs two recovery steps:

# 

# ```text

# Load SSTables

# &#x20;     |

# &#x20;     v

# Replay WAL

# &#x20;     |

# &#x20;     v

# Restore latest in-memory state

# ```

# 

# The database first loads existing SSTable data and then replays operations recorded in the WAL.

# 

# This helps restore operations that were recorded but had not yet been flushed to an SSTable.

# 

# \---

# 

# \## Benchmark

# 

# The program contains a simple benchmark using:

# 

# ```text

# 100,000 Put operations

# 100,000 Get operations

# ```

# 

# The benchmark measures:

# 

# \- Total execution time

# \- Queries Per Second (QPS)

# 

# QPS is calculated as:

# 

# ```text

# QPS = Number of Operations / Execution Time

# ```

# 

# Example output:

# 

# ```text

# \--- MiniLSM Benchmark Start ---

# 

# Put 100000 records: 2.45 seconds, QPS: 40816.3

# 

# Get 100000 records: 1.20 seconds, QPS: 83333.3

# 

# \--- Benchmark End ---

# ```

# 

# Actual performance depends on the computer, compiler, operating system, storage device, and existing database files.

# 

# \---

# 

# \## Project Structure

# 

# ```text

# MiniLSM/

# │

# ├── mini\_lsm.cpp

# ├── README.md

# ├── .gitignore

# └── .gitattributes

# ```

# 

# When the program runs, additional files are generated automatically:

# 

# ```text

# mini\_lsm.wal

# mini\_lsm\_0.sst

# mini\_lsm\_1.sst

# mini\_lsm\_2.sst

# ...

# ```

# 

# These generated database files do not need to be uploaded to GitHub.

# 

# \---

# 

# \## Requirements

# 

# A C++ compiler with C++17 support is recommended.

# 

# Examples:

# 

# \- GCC / g++

# \- MinGW-w64

# \- Clang

# \- Microsoft Visual C++

# 

# \---

# 

# \## Build

# 

# Using g++:

# 

# ```bash

# g++ -std=c++17 -O2 -pthread mini\_lsm.cpp -o mini\_lsm

# ```

# 

# \---

# 

# \## Run

# 

# \### Linux / macOS

# 

# ```bash

# ./mini\_lsm

# ```

# 

# \### Windows

# 

# ```bash

# mini\_lsm.exe

# ```

# 

# \---

# 

# \## Example Usage

# 

# A simple example using the database:

# 

# ```cpp

# DB db;

# 

# db.Put("username", "ChunXiang");

# 

# std::string value;

# 

# Status status = db.Get("username", \&value);

# 

# if (status.ok()) {

# &#x20;   std::cout << value << std::endl;

# }

# 

# db.Delete("username");

# ```

# 

# \---

# 

# \## Concepts Demonstrated

# 

# This project demonstrates several database and systems programming concepts:

# 

# \- C++

# \- Object-Oriented Programming

# \- Key-value databases

# \- LSM-tree architecture

# \- MemTables

# \- SSTables

# \- Write-Ahead Logging

# \- Crash recovery

# \- Tombstones

# \- Compaction

# \- File I/O

# \- Persistent storage

# \- Mutex-based synchronization

# \- Performance benchmarking

# \- Queries Per Second (QPS)

# 

# \---

# 

# \## Limitations

# 

# MiniLSM is an educational implementation and is \*\*not intended for production use\*\*.

# 

# The current implementation is simplified and does not include many features used by production database systems.

# 

# Current limitations include:

# 

# \- Sequential SSTable lookup

# \- Simple text-based SSTable format

# \- Basic WAL format

# \- Basic recovery logic

# \- Basic compaction strategy

# \- No Bloom filters

# \- No SSTable index

# \- No block cache

# \- No checksums

# \- No background compaction

# \- Limited concurrency

# \- No advanced transaction support

# \- No advanced corruption handling

# 

# \---

# 

# \## Future Improvements

# 

# Possible future improvements include:

# 

# \- Bloom filters

# \- SSTable indexing

# \- Binary SSTable format

# \- Block-based storage

# \- Block cache

# \- Checksums

# \- Improved WAL durability

# \- Background flushing

# \- Background compaction

# \- Leveled compaction

# \- Size-tiered compaction

# \- Configurable MemTable size

# \- Better SSTable file management

# \- Concurrent reads and writes

# \- Range queries

# \- Iterator support

# \- Performance profiling

# \- Automated unit testing

# \- Benchmark comparison with LevelDB or RocksDB

# 

# \---

# 

# \## Technologies

# 

# \- C++

# \- C++ Standard Library

# \- STL `std::map`

# \- STL `std::mutex`

# \- File I/O

# \- C++ Chrono

# \- g++

# 

# \---

# 

# \## Why I Built This

# 

# The goal of this project is to understand how a persistent storage engine works internally instead of treating a database as a black box.

# 

# By implementing a simplified LSM-tree storage engine from scratch, this project explores how databases:

# 

# \- accept writes

# \- store data in memory

# \- persist data to disk

# \- recover after restarting

# \- handle deleted data

# \- merge old data

# \- retrieve stored values

# \- measure storage-engine performance

# 

# The project is inspired by storage-engine concepts used in systems such as \*\*LevelDB\*\* and \*\*RocksDB\*\*, while intentionally keeping the implementation small enough for learning and experimentation.

# 

# \---

# 

# \## Author

# 

# \*\*Kow Chun Xiang\*\*

# 

# BSc Mathematics \& Applied Mathematics (Honours)  

# Xiamen University Malaysia

