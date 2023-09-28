/*

scanf implementation test program
Copyright (C) 2021 Sampo Hippeläinen (hisahi)

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include <algorithm>
#include <any>
#include <iomanip>
#include <iostream>
#include <tuple>

#include <cctype>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "wide.h"
#include "../scanf.h"
#include "../wscanf.h"

namespace test {
#define SCANF_WIDE 3
#include "../scanf.c"
extern "C" {
int getch_() {
    return -1;
}

void ungetch_(int) { }

WINT getwch_() {
    return WEOF;
}

void ungetwch_(WINT) { }

int scnext_(int (*getch)(void *data), void *data, const char **format,
            int *buffer, int length, int nostore, void *destination) {
    return 1;
}

void mbsetup_(scanf_mbstate_t *ps) {
    *ps = scanf_mbstate_t{};
}

constexpr size_t W_INVALID = static_cast<size_t>(-1);
constexpr size_t W_MORE = static_cast<size_t>(-2);

size_t mbrtowc_(WCHAR *pwc, const char *s, size_t n, scanf_mbstate_t *ps) {
    for (size_t i = 0; i < n; ++i) {
        char v = s[i];
        if (ps->left) {
            if ((v & 0xC0U) != 0x80U)
                return W_INVALID;
            ps->next = (ps->next << 6) | (v & 0x3FU);
            if (!--ps->left) {
                *pwc = ps->next;
                return i + 1; 
            }
        } else if (!(v & 0x80U)) {
            *pwc = (WCHAR)v;
            return v ? 1 : 0;
        } else if ((v & 0xE0U) == 0xC0U) {
            ps->next = v & 0x1FU;
            ps->left = 1;
        } else if ((v & 0xF0U) == 0xE0U) {
            ps->next = v & 0x0FU;
            ps->left = 2;
        } else if ((v & 0xF8U) == 0xF0U) {
            ps->next = v & 0x07U;
            ps->left = 3;
        } else
            return W_INVALID;
    }
    return W_MORE;
}

size_t wcrtomb_(char *s, WCHAR wc, scanf_mbstate_t *ps) {
    if (wc >= 0x110000)
        return W_INVALID;
    else if (wc >= 0x10000) {
        *s++ = (char)(0xF0U | ((wc >> 18) & 0x07U));
        *s++ = (char)(0x80U | ((wc >> 12) & 0x3FU));
        *s++ = (char)(0x80U | ((wc >>  6) & 0x3FU));
        *s++ = (char)(0x80U | ((wc >>  0) & 0x3FU));
        return 4;
    } else if (wc >= 0x0800) {
        *s++ = (char)(0xE0U | ((wc >> 12) & 0x0FU));
        *s++ = (char)(0x80U | ((wc >>  6) & 0x3FU));
        *s++ = (char)(0x80U | ((wc      ) & 0x3FU));
        return 3;
    } else if (wc >= 0x80) {
        *s++ = (char)(0xC0U | ((wc >>  6) & 0x1FU));
        *s++ = (char)(0x80U | ((wc      ) & 0x3FU));
        return 2;
    } else {
        *s++ = (char)(        ((wc      ) & 0x7FU));
        return 1;
    }
}

}

}

class TestCase {
public:
    virtual bool run() = 0;
    virtual const std::string& name() const = 0;
    virtual void whats_wrong(std::ostream& os) = 0;
};

template <std::size_t I>
struct buf_char {
    char s[I];  
    buf_char() {
        std::strncpy(s, "", I);
    }
    buf_char(const char* c) {
        std::strncpy(s, c, I);
    }
    bool operator==(const buf_char<I>& other) const {
        return 0 == std::strncmp(s, other.s, I);
    }
};

template <std::size_t I>
struct buf_str {
    char s[I];
    buf_str() {
        std::strncpy(s, "", I);
    }
    buf_str(const char* c) {
        if (std::strlen(c) > I)
            std::terminate();
        std::strncpy(s, c, I);
    }
    bool operator==(const buf_str<I>& other) const {
        return 0 == std::strncmp(s, other.s, I);
    }
};

std::size_t u32len(const char32_t* s) {
    std::size_t l = 0;
    while (*s)
        ++s, ++l;
    return l;
}

void u32ncpy(char32_t* d, const char32_t* s, std::size_t n) {
    for (std::size_t i = 0; i < n && (*d++ = *s++); ++i)
        ;
}

int u32ncmp(const char32_t* a, const char32_t* b, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i)
        if (a[i] > b[i])
            return 1;
        else if (a[i] < b[i])
            return -1;
        else if (!a[i])
            break;
    return 0;
}

template <std::size_t I>
struct buf_wstr {
    WCHAR s[I];
    buf_wstr() {
        u32ncpy(s, MAKE_WSTRING(""), I);
    }
    buf_wstr(const WCHAR* c) {
        if (u32len(c) > I)
            std::terminate();
        u32ncpy(s, c, I);
    }
    bool operator==(const buf_wstr<I>& other) const {
        return 0 == u32ncmp(s, other.s, I);
    }
};

template <std::size_t I>
std::ostream& operator<<(std::ostream& os, const buf_char<I>& buf) {
    return os << std::string(buf.s, I);
}

template <std::size_t I>
std::ostream& operator<<(std::ostream& os, const buf_str<I>& buf) {
    return os << buf.s;
}

std::ostream& operator<<(std::ostream& os, const std::u32string& s) {
    std::string r;
    char tmp[5];
    size_t i, n;
    scanf_mbstate_t mb{};
    for (char32_t c: s) {
        n = wcrtomb_(tmp, c, &mb);
        if (n == test::W_INVALID)
            std::terminate();
        for (i = 0; i < n; ++i)
            os << tmp[i];
    }
    return os;
}

template <std::size_t I>
std::ostream& operator<<(std::ostream& os, const buf_wstr<I>& buf) {
    return os << std::u32string(buf.s);
}

template <typename T>
bool compareEq(const T& a, const T& b) {
    return a == b;
}

template <>
bool compareEq<float>(const float& a, const float& b) {
    return a == b || (std::isnan(a) && std::isnan(b));
}

template <>
bool compareEq<double>(const double& a, const double& b) {
    return a == b || (std::isnan(a) && std::isnan(b));
}

/* with format string fmt and scanf return value q, get the number of
   values actually assigned for handling %n, %*, etc. */
int adjust_assigned(int q, const std::string& fmt) {
    int a = 0;
    bool percent = false;
    for (char c: fmt) {
        if (percent) {
            if (c == 'n') {
                ++a;
            } else if (c != '%' && c != '*') {
                if (!q) break;
                --q;
                ++a;
            }
            percent = false;
        } else if (c == '%') {
            percent = true;
        }
    }
    return a;
}


/* with format string fmt and scanf return value q, get the number of
   values actually assigned for handling %n, %*, etc. */
int adjust_assigned(int q, const std::u32string& fmt) {
    int a = 0;
    bool percent = false;
    for (char32_t c: fmt) {
        if (percent) {
            if (c == MAKE_WCHAR('n')) {
                ++a;
            } else if (c != MAKE_WCHAR('%') && c != MAKE_WCHAR('*')) {
                if (!q) break;
                --q;
                ++a;
            }
            percent = false;
        } else if (c == MAKE_WCHAR('%')) {
            percent = true;
        }
    }
    return a;
}

template <typename S, typename I, typename... Ts>
class ScanfTestCase_ : public TestCase {
    using char_type = typename S::value_type;

    template <std::size_t J>
    bool column_ok() const {
        return J >= assigned_
            || compareEq(std::get<J>(ret_), std::get<J>(comp_));
    }

    template <std::size_t J>
    bool column_ok_or_print(std::ostream& os) const {
        bool ok = std::get<J>(ret_) == std::get<J>(comp_);
        if (!ok) {
            os << "Input:  <" << input_ << ">" << std::endl;
            os << "Format: <" << format_ << ">" << std::endl;
            os << "Value #" << (J + 1) << " was wrong" << std::endl;
            os << "Returned: " << readin_ << std::endl;
            os << "Assigned: " << assigned_ << std::endl;
            os << "Type: " << typeid(typename std::tuple_element<J,
                        decltype(ret_)>::type).name() << std::endl;
            os << "Expected value: <" << std::get<J>(ret_)
                                      << ">" << std::endl;
            os << "     Got value: <" << std::get<J>(comp_)
                                      << ">" << std::endl;
        }
        return ok;
    }

    template <std::size_t... Is>
    bool whats_wrong_column(std::ostream& os,
                            std::index_sequence<Is...>) const {
        return (column_ok_or_print<Is>(os) && ...);
    }

    template <std::size_t... Is>
    bool run_(std::index_sequence<Is...>) {
        const char_type *s = input_.c_str();
        const char_type *f = format_.c_str();
        const char_type *sp = s;
        int n;
        if constexpr (!std::is_same_v<char_type, char>)
            n = test::spwscanf_(&sp, f, &std::get<Is>(comp_)...);
        else
            n = test::spscanf_(&sp, f, &std::get<Is>(comp_)...);
        int c = sp - s;
        readin_ = n;
        consumed_ = c;
        assigned_ = adjust_assigned(readin_, format_);
        return n == fields_ && c == consumed_ && (column_ok<Is>() && ...);
    }

public:
    bool run() {
        return run_(I{});
    }

    const std::string& name() const {
        return name_;
    }
    
    void whats_wrong(std::ostream& os) {
        if (readin_ != fields_) {
            os << "Expected " << fields_ << " field(s) to be read OK,\n" <<
                  "but the return value was " << readin_ << std::endl;
        } else if (consume_ != consumed_) {
            os << "Expected " << consume_ << " char(s) to be consumed,\n" <<
                  "but scanf consumed " << consumed_ << " chars" << std::endl;
        } else {
            whats_wrong_column(os, I{});
        }
    }

    ScanfTestCase_(const std::string& name, const S& input,
                int scanf_return_value, int scanf_chars_consumed,
                const S& fmt, const std::tuple<Ts...>& returns)
        : name_(name), input_(input), fields_(scanf_return_value),
          consume_(scanf_chars_consumed), format_(fmt), ret_(returns),
          readin_(0), comp_() { }

private:
    std::string name_;
    S input_;
    int fields_;
    int consume_;
    S format_;
    std::tuple<Ts...> ret_;
    int readin_;
    std::tuple<Ts...> comp_;
    int consumed_{0};
    int assigned_{0};
};

template <typename S, typename... Ts>
class ScanfTestCase :
    public ScanfTestCase_<S, std::make_index_sequence<sizeof...(Ts)>, Ts...> {
public:
    ScanfTestCase(const std::string& name, const S& input, int n, int c,
            const S& fmt, const std::tuple<Ts...>& returns)
        : ScanfTestCase_<S, std::make_index_sequence<sizeof...(Ts)>, Ts...>(
            name, input, n, c, fmt, returns) { }
};

template <typename S, typename... Ts>
bool run_test_(const std::string& name, int n, int c, const S& inp,
            const S& fmt, Ts&&... values) {
    auto test = ScanfTestCase<S, Ts...>(name, inp, n, c, fmt,
            std::make_tuple(std::forward<Ts>(values)...));
    std::cout << std::left << std::setw(70) << name
                           << std::setw(0) << std::flush;
    bool flag = test.run();
    if (!flag) {
        std::cout << "FAIL" << std::endl;
        test.whats_wrong(std::cout);
        return false;
    }
    std::cout << "OK  " << std::endl;
    return flag;
}

template <typename... Ts>
bool run_test(const std::string& name, int n, int c, const std::string& inp,
            const std::string& fmt, Ts&&... values) {
    return run_test_<std::string>(name, n, c, inp, fmt,
                                std::forward<Ts>(values)...);
}

template <typename... Ts>
bool run_test(const std::string& name, int n, int c, const std::u32string& inp,
            const std::u32string& fmt, Ts&&... values) {
    return run_test_<std::u32string>(name, n, c, inp, fmt,
                                     std::forward<Ts>(values)...);
}

#define TRY_TEST(...) do {                                                     \
        if (!run_test(__VA_ARGS__))                                            \
            return false;                                                      \
        ++tests;                                                               \
    } while (0);

/* 1, 2, "42", "%d", int(42)... =
    {
        int t0;
        1 == sscanf("42", "%d", &t0) && t0 == 42 && scanf consumed 2 chars;
    }
*/
#define C(x) MAKE_WCHAR(x)
#define S(x) MAKE_WSTRING(x)
#define U8(x) (reinterpret_cast<const char *>(u8##x))

bool tests_ok() {
    std::cout << "Running wscanf tests..." << std::endl;
    int tests = 0;

    TRY_TEST("Simple wide=>wide %ls",
        1, 6, S("привет"), S("%ls"), buf_wstr<21>(S("привет")));

    TRY_TEST("Simple narrow=>wide %ls",
        1, 12, U8("привет"), "%ls", buf_wstr<21>(S("привет")));

    TRY_TEST("Simple wide=>narrow %ls",
        1, 6, S("привет"), S("%s"), buf_str<31>(U8("привет")));

    std::cout << "==========\n"
                 "  ALL OK  \n"
                 "==========" << std::endl;
    std::cout << "Ran " << tests << " tests" << std::endl;
    return true;
}

int main(int argc, char* argv[]) {
    return tests_ok() ? EXIT_SUCCESS : EXIT_FAILURE;
}
