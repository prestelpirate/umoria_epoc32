/* Minimal std::string / ostringstream / to_string for EPOC (no libstdc++).
 * Only what umoria uses. A zero-filled object is a valid empty string, so
 * globals work even if static constructors were never run. */
#ifndef EPOC_ESTRING_H
#define EPOC_ESTRING_H
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

class string {
    char *p_;
    size_t n_;
    void set(const char *s, size_t n) {
        char *q = (char *) malloc(n + 1);
        if (n) memcpy(q, s, n);
        q[n] = 0;
        free(p_);
        p_ = q; n_ = n;
    }
public:
    string() : p_(0), n_(0) {}
    string(const char *s) : p_(0), n_(0) { if (s) set(s, strlen(s)); }
    string(const char *s, size_t n) : p_(0), n_(0) { set(s, n); }
    string(size_t n, char c) : p_(0), n_(0) { set("", 0); if (n) { char *q = (char *) malloc(n + 1); memset(q, c, n); q[n] = 0; free(p_); p_ = q; n_ = n; } }
    string(const string &o) : p_(0), n_(0) { if (o.n_ || o.p_) set(o.c_str(), o.n_); }
    ~string() { free(p_); }
    string &operator=(const string &o) { if (this != &o) set(o.c_str(), o.n_); return *this; }
    string &operator=(const char *s) { set(s ? s : "", s ? strlen(s) : 0); return *this; }
    const char *c_str() const { return p_ ? p_ : ""; }
    const char *data() const { return c_str(); }
    size_t length() const { return n_; }
    size_t size() const { return n_; }
    bool empty() const { return n_ == 0; }
    char &operator[](size_t i) { return p_[i]; }
    char operator[](size_t i) const { return c_str()[i]; }
    string &operator+=(const string &o) { return append(o.c_str(), o.n_); }
    string &operator+=(const char *s) { return append(s, strlen(s)); }
    string &operator+=(char c) { return append(&c, 1); }
    string &append(const char *s, size_t n) {
        char *q = (char *) malloc(n_ + n + 1);
        if (n_) memcpy(q, p_, n_);
        memcpy(q + n_, s, n);
        q[n_ + n] = 0;
        free(p_);
        p_ = q; n_ += n;
        return *this;
    }
    void resize(size_t n, char c = 0) {
        if (n <= n_) { if (p_) p_[n] = 0; n_ = n; return; }
        string pad(n - n_, c); append(pad.c_str(), pad.length());
    }
    string substr(size_t pos, size_t len = (size_t) -1) const {
        if (pos > n_) pos = n_;
        if (len > n_ - pos) len = n_ - pos;
        return string(c_str() + pos, len);
    }
};

inline string operator+(const string &a, const string &b) { string r(a); r += b; return r; }
inline string operator+(const string &a, const char *b) { string r(a); r += b; return r; }
inline string operator+(const char *a, const string &b) { string r(a); r += b; return r; }
inline string operator+(const string &a, char b) { string r(a); r += b; return r; }
inline bool operator==(const string &a, const string &b) { return a.length() == b.length() && strcmp(a.c_str(), b.c_str()) == 0; }
inline bool operator==(const string &a, const char *b) { return strcmp(a.c_str(), b) == 0; }
inline bool operator!=(const string &a, const string &b) { return !(a == b); }
inline bool operator!=(const string &a, const char *b) { return !(a == b); }

inline string to_string(long v) { char b[24]; sprintf(b, "%ld", v); return string(b); }
inline string to_string(int v) { return to_string((long) v); }
inline string to_string(unsigned long v) { char b[24]; sprintf(b, "%lu", v); return string(b); }
inline string to_string(unsigned int v) { return to_string((unsigned long) v); }

class ostringstream {
    string s_;
public:
    ostringstream &operator<<(const string &v) { s_ += v; return *this; }
    ostringstream &operator<<(const char *v) { s_ += v; return *this; }
    ostringstream &operator<<(char v) { s_ += v; return *this; }
    ostringstream &operator<<(long v) { s_ += to_string(v); return *this; }
    ostringstream &operator<<(int v) { s_ += to_string((long) v); return *this; }
    ostringstream &operator<<(unsigned long v) { s_ += to_string(v); return *this; }
    ostringstream &operator<<(unsigned int v) { s_ += to_string((unsigned long) v); return *this; }
    string str() const { return s_; }
};

/* cerr: discard (there is no console); plog-style messages go through the UI */
struct epoc_null_stream {
    template <class T> epoc_null_stream &operator<<(const T &) { return *this; }
};
extern epoc_null_stream cerr;
#endif
