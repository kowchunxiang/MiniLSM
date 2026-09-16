# MiniLSM

MiniLSM is a lightweight **LSM-tree inspired key-value storage engine** written in C++.

The project demonstrates several fundamental concepts used in modern database systems, including **Write-Ahead Logging (WAL), MemTables, SSTables, tombstones, compaction, crash recovery, thread synchronization, and performance benchmarking**.

This project is intended for educational purposes and for understanding how persistent key-value databases work internally.

---

## Features

MiniLSM currently supports:

- `Put(key, value)` — insert or update a key-value pair
- `Get(key)` — retrieve a value using a key
- `Delete(key)` — logically delete a key using a tombstone
- Write-Ahead Log (WAL)
- Basic crash recovery
- In-memory MemTable
- SSTable persistence
- Automatic MemTable flushing
- Basic SSTable compaction
- Tombstone removal during compaction
- Thread synchronization using `std::mutex`
- Put/Get performance benchmark

---

## Architecture

The basic write flow is:

```text
Put(key, value)
       |
       v
Write-Ahead Log
       |
       v
MemTable
       |
       | MemTable reaches limit
       v
SSTable
       |
       | Multiple SSTables
       v
Compaction
```

The read flow is:

```text
Get(key)
   |
   v
MemTable
   |
   | Not Found
   v
Newest SSTable
   |
   v
Older SSTables
```

---

## How It Works

### 1. Write-Ahead Log

Before a `Put` or `Delete` operation is stored in memory, the operation is written to:

```text
mini_lsm.wal
```

Example WAL entry for a Put operation:

```text
PUT
key_1
value_1
```

Example WAL entry for a Delete operation:

```text
DELETE
key_1
```

If the program restarts, MiniLSM reads the WAL and replays the recorded operations.

This provides a simple form of crash recovery.

---

### 2. MemTable

Recent data is stored in memory using:

```cpp
std::map<std::string, std::string>
```

The MemTable stores new and updated values before they are written to disk.

The maximum MemTable size is currently:

```text
10,000 records
```

When the MemTable reaches this limit, the data is flushed into an SSTable.

---

### 3. SSTable

An SSTable is an immutable file used to store key-value data on disk.

MiniLSM generates files such as:

```text
mini_lsm_0.sst
mini_lsm_1.sst
mini_lsm_2.sst
...
```

Each entry is stored as:

```text
key
value
```

After the MemTable is successfully flushed:

1. The SSTable is written to disk
2. The MemTable is cleared
3. The WAL is cleared
4. A new WAL is opened

---

### 4. Put

The `Put` operation performs the following steps:

```text
Put(key, value)
      |
      v
Validate key
      |
      v
Lock mutex
      |
      v
Write operation to WAL
      |
      v
Store value in MemTable
      |
      v
Check MemTable size
      |
      v
Flush to SSTable if necessary
```

Example:

```cpp
db.Put("name", "Alice");
```

---

### 5. Get

The `Get` operation first searches the in-memory MemTable.

If the key is not found there, MiniLSM searches SSTables from newest to oldest.

This is important because newer SSTables may contain newer versions of the same key.

Example:

```cpp
std::string value;

Status status = db.Get("name", &value);
```

If the key exists:

```text
value = Alice
```

If the key does not exist, MiniLSM returns:

```text
NotFound
```

---

### 6. Delete

MiniLSM uses a **tombstone** instead of immediately removing a key.

The tombstone value is:

```text
__TOMBSTONE__
```

Example:

```cpp
db.Delete("name");
```

Internally:

```text
name -> __TOMBSTONE__
```

When `Get()` finds a tombstone, the key is treated as deleted.

The tombstone can later be permanently removed during compaction.

---

## Compaction

When several SSTables have been created, MiniLSM performs compaction.

During compaction, MiniLSM:

- Reads existing SSTables
- Keeps the newest version of each key
- Removes older duplicate versions
- Removes tombstones
- Deletes old SSTable files
- Creates a new merged SSTable

Conceptually:

```text
SSTable 0
    +
SSTable 1
    +
SSTable 2
    |
    v
Compaction
    |
    v
Merged SSTable
```

Example:

Before compaction:

```text
SSTable 0:
A -> 10
B -> 20

SSTable 1:
A -> 30
C -> 40

SSTable 2:
B -> __TOMBSTONE__
```

After compaction:

```text
A -> 30
C -> 40
```

The old value of `A` is removed and the deleted key `B` is permanently removed.

---

## Thread Safety

MiniLSM uses:

```cpp
std::mutex
```

and:

```cpp
std::lock_guard<std::mutex>
```

to protect shared in-memory data.

This prevents multiple threads from modifying the MemTable at the same time during protected operations.

---

## Status Handling

The project contains a simple `Status` class for reporting operation results.

Current status codes include:

```cpp
kOk
kNotFound
kInvalidArgument
```

Examples:

```cpp
Status::OK();
Status::NotFound();
Status::InvalidArgument("key is empty");
```

---

## Crash Recovery

When MiniLSM starts, it performs two recovery steps:

```text
Load SSTables
      |
      v
Replay WAL
      |
      v
Restore latest in-memory state
```

The database first loads existing SSTable data and then replays operations recorded in the WAL.

This helps restore operations that were recorded but had not yet been flushed to an SSTable.

---

## Benchmark

The program contains a simple benchmark using:

```text
100,000 Put operations
100,000 Get operations
```

The benchmark measures:

- Total execution time
- Queries Per Second (QPS)

QPS is calculated as:

```text
QPS = Number of Operations / Execution Time
```

Example output:

```text
--- MiniLSM Benchmark Start ---

Put 100000 records: X seconds, QPS: XXXXX

Get 100000 records: X seconds, QPS: XXXXX

--- Benchmark End ---
```

Actual performance depends on the computer, compiler, operating system, storage device, and existing database files.

---

## Project Structure

```text
MiniLSM/
├── mini_lsm.cpp
├── README.md
├── .gitignore
└── .gitattributes
```

When the program runs, additional files are generated automatically:

```text
mini_lsm.wal
mini_lsm_0.sst
mini_lsm_1.sst
mini_lsm_2.sst
...
```

These generated database files do not need to be uploaded to GitHub.

---

## Requirements

A C++ compiler with C++17 support is recommended.

Examples:

- GCC / g++
- MinGW-w64
- Clang
- Microsoft Visual C++

---

## Build

Using g++:

```bash
g++ -std=c++17 -O2 -pthread mini_lsm.cpp -o mini_lsm
```

---

## Run

### Linux / macOS

```bash
./mini_lsm
```

### Windows

```bash
mini_lsm.exe
```

---

## Example Usage

```cpp
DB db;

db.Put("username", "ChunXiang");

std::string value;

Status status = db.Get("username", &value);

if (status.ok()) {
    std::cout << value << std::endl;
}

db.Delete("username");
```

---

## Concepts Demonstrated

This project demonstrates several database and systems programming concepts:

- C++
- Object-Oriented Programming
- Key-value databases
- LSM-tree architecture
- MemTables
- SSTables
- Write-Ahead Logging
- Crash recovery
- Tombstones
- Compaction
- File I/O
- Persistent storage
- Mutex-based synchronization
- Performance benchmarking
- Queries Per Second (QPS)

---

## Limitations

MiniLSM is an educational implementation and is **not intended for production use**.

Current limitations include:

- Sequential SSTable lookup
- Simple text-based SSTable format
- Basic WAL format
- Basic recovery logic
- Basic compaction strategy
- No Bloom filters
- No SSTable index
- No block cache
- No checksums
- No background compaction
- Limited concurrency
- No transaction support

---

## Future Improvements

Possible future improvements include:

- Bloom filters
- SSTable indexing
- Binary SSTable format
- Block cache
- Checksums
- Improved WAL durability
- Background flushing
- Background compaction
- Leveled compaction
- Configurable MemTable size
- Better SSTable file management
- Concurrent reads and writes
- Range queries
- Iterator support
- Performance profiling
- Automated unit testing
- Benchmark comparison with LevelDB or RocksDB

---

## Technologies

- C++
- C++ Standard Library
- STL `std::map`
- STL `std::mutex`
- File I/O
- C++ Chrono
- g++

---

## Why I Built This

The goal of this project is to understand how a persistent storage engine works internally instead of treating a database as a black box.

By implementing a simplified LSM-tree storage engine from scratch, this project explores how databases:

- accept writes
- store data in memory
- persist data to disk
- recover after restarting
- handle deleted data
- merge old data
- retrieve stored values
- measure storage-engine performance

The project is inspired by storage-engine concepts used in systems such as **LevelDB** and **RocksDB**, while intentionally keeping the implementation small enough for learning and experimentation.

---

## Author

**Kow Chun Xiang**

BSc Mathematics & Applied Mathematics (Honours)  
Xiamen University Malaysia
