//
// Created by Fernando on 16-Nov-16.
//
#ifndef DB_CONNECTION_HPP
#define DB_CONNECTION_HPP

#include <sqlite3.h>
#include <string>

//Concepts
#define Callable typename

namespace bitprim {

class db_connection {
public:
    explicit
    db_connection(char const* name) {
        int rc = sqlite3_open(name, &db_);
        //TODO: Fer: do something with rc
    }

    explicit
    db_connection(std::string const& name) {
        int rc = sqlite3_open(name.c_str(), &db_);
        //TODO: Fer: do something with rc
    }

    ~db_connection() {
        //TODO: What happens if sqlite3_open results in error?
        sqlite3_close(db_);
    }

    template <Callable C1, Callable C2>
    int exec_each(char const* sql, C1 each, C2 result) const {
        char* error_msg_z = nullptr;
        auto res = exec_helper(sql, &error_msg_z, each);

        //TODO: Fer: use a string_view or something like that
        std::string error_msg = error_msg_z == nullptr ? "" : error_msg_z;
        if (res != SQLITE_OK) {
            sqlite3_free(error_msg_z);
        }

        result(res, error_msg);
        return res;
    }

    template <Callable C1>
    int exec_each(char const* sql, C1 each) const {
        return exec_helper(sql, nullptr, each);
    }

    template <Callable C1>
    int exec(char const* sql, C1 result) const {
        char* error_msg_z = nullptr;
        auto res = exec_helper(sql, &error_msg_z);

        //TODO: Fer: use a string_view or something like that
        std::string error_msg = error_msg_z == nullptr ? "" : error_msg_z;
        if (res != SQLITE_OK) {
            sqlite3_free(error_msg_z);
        }

        result(res, error_msg);
        return res;
    }

    int exec(char const* sql) const {
        return exec_helper(sql, nullptr);
    }

    sqlite3* ptr() const {
        return db_;
    }

private:
    int exec_helper(char const* sql, char** error_msg_z) const {
        return sqlite3_exec(db_, sql, nullptr, nullptr, error_msg_z);
    }

    template <Callable C1>
    int exec_helper(char const* sql, char** error_msg_z, C1 each) const {
        return sqlite3_exec(db_, sql,
                                [](void* ptr, int argc, char** argv, char** azColName) {
                                    auto&& f = *static_cast<C1*>(ptr);
                                    return f(argc, argv, azColName);
                                },
                                static_cast<void*>(&each), error_msg_z);
    }

    sqlite3* db_;
};

inline
int exec(db_connection const& db, char const* sql) {
    return db.exec(sql);
}

template <Callable C1>
int exec(db_connection const& db, char const* sql, C1 result) {
    return db.exec(sql, result);
}

template <Callable C1, Callable C2>
int exec_each(db_connection const& db, char const* sql, C1 each, C2 result) {
    return db.exec_each(sql, each, result);
}

template <Callable C1>
int exec_each(db_connection const& db, char const* sql, C1 each) {
    return db.exec_each(sql, each);
}

} /*namespace bitprim*/

#undef Callable

#endif /*DB_CONNECTION_HPP*/
