#ifndef _NANOPARSECISH_H
#define _NANOPARSECISH_H

#include <experimental/optional>
#include <vector>
#include <functional>
#include <algorithm>
#include <memory>

#define PARSER(T) std::function<std::vector<ParseProduct<T>>(std::string)>

template <typename T>
struct ParseProduct {
    T value;
    std::string rest;
};

template <typename T>
std::experimental::optional<T> runParser(PARSER(T) parseFun, std::string toParse) {
    auto result = parseFun(toParse);
    if (result.size() != 1) {
        return std::experimental::optional<T>();
    } else if (result[0].rest.size() != 0) { // Did not consume whole string.
        return std::experimental::optional<T>();
    }
    return std::experimental::optional<T>(result[0].value);
}

template <typename T>
PARSER(T) unit(T a) {
    return [a](std::string s) {
        ParseProduct<T> prod;
        prod.value = a;
        prod.rest = s;
        return std::vector<ParseProduct<T>> { prod };
    };
}

template <typename T>
PARSER(T) failure() {
    return [](std::string s) {
        return std::vector<ParseProduct<T>>();
    };
}

PARSER(char) item() {
    return [](std::string s) {
        if (s.size() == 0) {
            return failure<char>()(s);
        }
        auto newS = s;
        newS.erase(0, 1);
        
        return unit<char>(s[0])(newS);
    };
}

template <typename T, typename U>
PARSER(U) bind(PARSER(T) p, std::function<PARSER(U)(T)> f) {
    return [p, f](std::string s) {
        auto pParse = p(s);
        std::vector<ParseProduct<U>> prodU;
        for (auto product : pParse) {
            auto fParse = f(product.value)(product.rest);
            prodU.insert(prodU.end(), fParse.begin(), fParse.end());
        }
        return prodU;
    };
}

template <typename T>
PARSER(T) combine(PARSER(T) p, PARSER(T) q) {
    return [p, q](std::string s) {
        auto pParse = p(s);
        auto qParse = q(s);
        pParse.insert(pParse.end(), qParse.begin(), qParse.end());
        return pParse;
    };
}

template <typename T>
PARSER(T) option(PARSER(T) p, PARSER(T) q) {
    return [p, q](std::string s) {
        auto pParse = p(s);
        if (pParse.size() == 0) {
            return q(s);
        }
        return pParse;
    };
}

template <typename T>
PARSER(std::vector<T>) many(PARSER(T) p) {
    return option
        (bind<T, std::vector<T>>
             (p,
              [p](T value) {
                  return bind<std::vector<T>, std::vector<T>>
                      (many<T>(p),
                       [value](std::vector<T> others) {
                           others.insert(others.begin(), value);
                           return unit<std::vector<T>>(others);
                       });
              })
        , unit<std::vector<T>>(std::vector<T>()));
}

template <typename T>
PARSER(std::vector<T>) some(PARSER(T) p) {
    return bind<T, std::vector<T>>
        (p,
         [p](T value) {
             return bind<std::vector<T>, std::vector<T>>
                 (many<T>(p),
                  [value](std::vector<T> others) {
                      others.insert(others.begin(), value);
                      return unit<std::vector<T>>(others);
                  });
         });
}

PARSER(char) satisfy(std::function<bool(char)> p) {
    return bind<char, char>(item(), [p](char c) {
        if (p(c)) {
            return unit<char>(c);
        }
        return failure<char>();
    });
}

PARSER(char) oneOf(std::vector<char> charList) {
    return satisfy([charList](char c) {
        return std::find(charList.begin(), charList.end(), c) != charList.end();
    });
}

// Only used by chainl1. Don't actually use this anywhere please.
template <typename T>
PARSER(T) rest(T a, PARSER(T) p, PARSER(std::function<T(T, T)>) op) {
    return option(bind<std::function<T(T, T)>, T>
                  (op,
                   [p, op, a](std::function<T(T, T)> opFun) {
                       return bind<T, T>
                           (p,
                            [p, op, opFun, a](T b) {
                                return rest(opFun(a, b), p, op);
                            });
                   })
        , unit<T>(a));
};

// Parse one or more occurrences of p separated by op. This can be used to parse left-recursive grammars.
template <typename T>
PARSER(T) chainl1(PARSER(T) p, PARSER(std::function<T(T, T)>) op) {
    return bind<T, T>
        (p,
         [p, op](T a) {
             return rest(a, p, op);
         });
}

// More interesting stuff:

PARSER(char) getChar(char c) {
    return satisfy([c](char cs) {
       return c == cs; 
    });
}

bool isDigit(char c) {
    std::string numbers = "0123456789";
    return std::find(numbers.begin(), numbers.end(), c) != numbers.end();
}

PARSER(char) digit() {
    return satisfy(isDigit);
}

PARSER(int) natural() {
    return bind<std::vector<char>, int>
        (some<char>(digit()),
             [](std::vector<char> numS) {
                 std::string value(numS.begin(), numS.end());
                 return unit<int>(std::stoi(value));
             });
}

PARSER(std::string) getString(std::string str) {
    if (str.size() == 0) {
        return unit<std::string>(std::string(""));
    }
    
    char curIn = str[0];
    str.erase(0, 1);
    
    return bind<char, std::string>
        (getChar(curIn),
         [str](char cur) {
             return bind<std::string, std::string>
                 (getString(str),
                  [cur](std::string rest) {
                      rest.insert(rest.begin(), cur);
                      return unit<std::string>(rest);
                  });
         });
}

PARSER(int) number() {
    return bind<std::string, int>
        (option(getString(std::string("-")), unit(std::string(""))),
         [](std::string sign) {
             return bind<std::vector<char>, int>
                 (some(digit()),
                  [sign](std::vector<char> numV) {
                      auto val = std::string(numV.begin(), numV.end());
                      return unit<int>(std::stoi(sign + val));
                  });
         });
}

PARSER(std::string) spaces() {
    return bind<std::vector<char>, std::string>
        (many<char>(oneOf(std::vector<char> { ' ', '\n', '\r', '\t' })),
             [](std::vector<char> chars) {
                 std::string str(chars.begin(), chars.end());
                 return unit<std::string>(str);
             });
}

template <typename T>
PARSER(T) token(PARSER(T) p) {
    return bind<T, T>
        (p,
         [](T value) {
             return bind<std::string, T>
                 (spaces(),
                  [value](std::string ws) {
                      return unit<T>(value);
                  });
         });
}

PARSER(std::string) reserved(std::string s) {
    return token(getString(s));
}

template <typename T>
PARSER(T) parens(PARSER(T) p) {
    return bind<std::string, T>
        (reserved("("),
         [p](std::string open) {
             return bind<T, T>
                 (p,
                  [](T value) {
                      return bind<std::string, T>
                          (reserved(")"),
                           [value](std::string close) {
                               return unit<T>(value);
                           });
                  });
         });
}

template <typename T>
PARSER(T) braces(PARSER(T) p) {
    return bind<std::string, T>
        (reserved("{"),
         [p](std::string open) {
             return bind<T, T>
                 (p,
                  [](T value) {
                      return bind<std::string, T>
                          (reserved("}"),
                           [value](std::string close) {
                               return unit<T>(value);
                           });
                  });
         });
}

template <typename T>
PARSER(T) squareBraces(PARSER(T) p) {
    return bind<std::string, T>
        (reserved("["),
         [p](std::string open) {
             return bind<T, T>
                 (p,
                  [](T value) {
                      return bind<std::string, T>
                          (reserved("]"),
                           [value](std::string close) {
                               return unit<T>(value);
                           });
                  });
         });
}

#endif

