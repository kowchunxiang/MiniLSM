#include <iostream>
#include <string>
#include <map>
#include <mutex>
#include <fstream>
#include <cstdio>
#include <chrono>

const std::string kTombstone = "__TOMBSTONE__";

class Status {
public:
    enum Code { kOk = 0, kNotFound = 1, kInvalidArgument = 2 };
    Status() : code_(kOk) {}
    Status(Code code, std::string msg = "") : code_(code), message_(std::move(msg)) {}
    static Status OK() { return Status(); }
    static Status NotFound() { return Status(kNotFound, "not found"); }
    static Status InvalidArgument(const std::string& msg) { return Status(kInvalidArgument, msg); }
    bool ok() const { return code_ == kOk; }
    bool IsNotFound() const { return code_ == kNotFound; }
private:
    Code code_;
    std::string message_;
};

class DB {
public:
    DB() : kMaxMemTableSize(10000), sst_counter_(0) {
        LoadAllSSTables();
        RecoverFromWAL();
        wal_file_.open("mini_lsm.wal", std::ios::app);
    }

    ~DB() {
        if (wal_file_.is_open()) wal_file_.close();
    }

    Status Put(const std::string& key, const std::string& value) {
        if (key.empty()) return Status::InvalidArgument("key is empty");
        std::lock_guard<std::mutex> lock(mutex_);
        wal_file_ << "PUT\n" << key << "\n" << value << "\n";
        data_[key] = value;
        if (data_.size() >= kMaxMemTableSize) FlushSSTable();
        return Status::OK();
    }

    Status Get(const std::string& key, std::string* value) {
        if (key.empty()) return Status::InvalidArgument("key is empty");
        if (value == nullptr) return Status::InvalidArgument("value is null");

        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = data_.find(key);
            if (it != data_.end()) {
                if (it->second == kTombstone) return Status::NotFound();
                *value = it->second;
                return Status::OK();
            }
        }

        for (int i = sst_counter_ - 1; i >= 0; --i) {
            std::ifstream sst_file("mini_lsm_" + std::to_string(i) + ".sst");
            if (!sst_file.is_open()) continue;
            std::string k, v;
            while (std::getline(sst_file, k) && std::getline(sst_file, v)) {
                if (k == key) {
                    if (v == kTombstone) return Status::NotFound();
                    *value = v;
                    return Status::OK();
                }
            }
        }
        return Status::NotFound();
    }

    Status Delete(const std::string& key) {
        if (key.empty()) return Status::InvalidArgument("key is empty");
        std::lock_guard<std::mutex> lock(mutex_);
        wal_file_ << "DELETE\n" << key << "\n";
        data_[key] = kTombstone;
        return Status::OK();
    }

private:
    void LoadAllSSTables() {
        while (true) {
            std::ifstream sst_file("mini_lsm_" + std::to_string(sst_counter_) + ".sst");
            if (!sst_file.is_open()) break;
            std::string key, value;
            while (std::getline(sst_file, key) && std::getline(sst_file, value)) {
                data_[key] = value;
            }
            sst_counter_++;
        }
    }

    void RecoverFromWAL() {
        std::ifstream in_file("mini_lsm.wal");
        if (!in_file.is_open()) return;
        std::string op;
        while (std::getline(in_file, op)) {
            if (op == "PUT") {
                std::string key, value;
                std::getline(in_file, key); std::getline(in_file, value);
                data_[key] = value;
            }
            else if (op == "DELETE") {
                std::string key; std::getline(in_file, key);
                data_[key] = kTombstone;
            }
        }
    }

    void FlushSSTable() {
        std::string filename = "mini_lsm_" + std::to_string(sst_counter_++) + ".sst";
        std::ofstream sst_file(filename, std::ios::trunc);
        for (const auto& kv : data_) sst_file << kv.first << "\n" << kv.second << "\n";
        sst_file.close();
        data_.clear();

        wal_file_.close();
        std::ofstream("mini_lsm.wal", std::ios::trunc).close();
        wal_file_.open("mini_lsm.wal", std::ios::app);

        if (sst_counter_ >= 3) Compact();
    }

    void Compact() {
        std::map<std::string, std::string> merged;
        for (int i = sst_counter_ - 1; i >= 0; --i) {
            std::ifstream in("mini_lsm_" + std::to_string(i) + ".sst");
            if (!in.is_open()) continue;
            std::string k, v;
            while (std::getline(in, k) && std::getline(in, v)) {
                if (merged.find(k) == merged.end()) merged[k] = v;
            }
            in.close();
            std::remove(("mini_lsm_" + std::to_string(i) + ".sst").c_str());
        }

        for (auto it = merged.begin(); it != merged.end(); ) {
            if (it->second == kTombstone) it = merged.erase(it);
            else ++it;
        }

        std::string new_name = "mini_lsm_" + std::to_string(sst_counter_++) + ".sst";
        std::ofstream out(new_name);
        for (const auto& kv : merged) out << kv.first << "\n" << kv.second << "\n";
        out.close();
    }

    std::mutex mutex_;
    std::map<std::string, std::string> data_;
    std::ofstream wal_file_;
    const size_t kMaxMemTableSize;
    int sst_counter_;
};

int main() {
    std::cout << "--- MiniLSM Benchmark Start ---" << std::endl;
    const int N = 100000;

    {
        DB db;
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < N; ++i) {
            db.Put("key_" + std::to_string(i), "value_" + std::to_string(i));
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = end - start;
        double qps = N / diff.count();
        std::cout << "Put " << N << " records: " << diff.count() << " seconds, QPS: " << qps << std::endl;
    }

    {
        DB db;
        std::string value;
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < N; ++i) {
            db.Get("key_" + std::to_string(i), &value);
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = end - start;
        double qps = N / diff.count();
        std::cout << "Get " << N << " records: " << diff.count() << " seconds, QPS: " << qps << std::endl;
    }

    std::cout << "--- Benchmark End ---" << std::endl;
    return 0;
}