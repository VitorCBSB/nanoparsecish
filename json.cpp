#include "nanoparsecish.h"
#include <iostream>
#include <vector>
#include <string>

// A parser for JSON as stated by http://json.org
// Once again, pay no mind to the egregious memory leaks.
// I'm lazy.

// -----

// Data definitions

struct JSONValue {
    virtual void print() = 0;
};

struct String : public JSONValue {
    std::string theString;

    void print() {
        std::cout << theString;
    }
};

struct Number : public JSONValue {
    int number;

    void print() {
        std::cout << number;
    }
};

struct Array : public JSONValue {
    std::vector<JSONValue*> content;

    void print() {
        for (auto elem : content) {
            elem->print();
            std::cout << std::endl;
        }
    }
};

struct Boolean : public JSONValue {
    bool value;

    void print() {
        std::cout << value;
    }
};

struct Null : public JSONValue {
    void print() {
        std::cout << "null";
    }
};

struct Pair {
    std::string name;
    JSONValue* value;
};

struct JSONObject : public JSONValue {
    std::vector<Pair> members;

    void print() {
        for (auto member : members) {
            std::cout << member.name << " : ";
            member.value->print();
            std::cout << std::endl;
        }
    }
};

// -----

// Parser definitions

PARSER(String*) jsonString();
PARSER(Number*) jsonNumber();
PARSER(Boolean*) boolean();
PARSER(Null*) null();
PARSER(Array*) array();
PARSER(JSONValue*) value();
PARSER(Pair) pair();
PARSER(JSONObject*) object();

bool isNotQuote(char c) {
    return c != '"';
}

// Damnit C++...
// Wrappers to transform JSON subclasses into their superclass JSONValue.

PARSER(JSONValue*) wrapString(PARSER(String*) p) {
    return bind<String*, JSONValue*>
        (p,
         [](String* v) {
            return unit<JSONValue*>(v);
         });
}

PARSER(JSONValue*) wrapNumber(PARSER(Number*) p) {
    return bind<Number*, JSONValue*>
        (p,
         [](Number* v) {
            return unit<JSONValue*>(v);
         });
}

PARSER(JSONValue*) wrapArray(PARSER(Array*) p) {
    return bind<Array*, JSONValue*>
        (p,
         [](Array* v) {
            return unit<JSONValue*>(v);
         });
}

PARSER(JSONValue*) wrapBoolean(PARSER(Boolean*) p) {
    return bind<Boolean*, JSONValue*>
        (p,
         [](Boolean* v) {
            return unit<JSONValue*>(v);
         });
}

PARSER(JSONValue*) wrapNull(PARSER(Null*) p) {
    return bind<Null*, JSONValue*>
        (p,
         [](Null* v) {
            return unit<JSONValue*>(v);
         });
}

PARSER(JSONValue*) wrapObject(PARSER(JSONObject*) p) {
    return bind<JSONObject*, JSONValue*>
        (p,
         [](JSONObject* v) {
            return unit<JSONValue*>(v);
         });
}

// Wrappers end here.

PARSER(String*) jsonString() {
    return token(bind<char, String*>
        (getChar('"'),
         [](char opQuote) {
            return bind<std::vector<char>, String*>
                (many<char>(satisfy(isNotQuote)),
                 [](std::vector<char> vecName) {
                    std::string name(vecName.begin(), vecName.end());
                    return bind<char, String*>
                        (getChar('"'),
                         [name](char clQuote) {
                            String* newString = new String();
                            newString->theString = name;
                            return unit<String*>(newString);
                         });
                 });
         }));
}

PARSER(Number*) jsonNumber() {
    return bind<int, Number*>
        (number(),
         [](int res) {
            Number* num = new Number();
            num->number = res;
            return unit<Number*>(num);
         });
}

PARSER(Boolean*) boolean() {
    return bind<std::string, Boolean*>
        (option(reserved("true"), reserved("false")),
         [](std::string boolString) {
            Boolean* newBoolean = new Boolean();
            if (boolString == "true") {
                newBoolean->value = true;
            } else {
                newBoolean->value = false;
            }
            return unit<Boolean*>(newBoolean);
         });
}

PARSER(Null*) null() {
    return bind<std::string, Null*>
        (reserved("null"),
         [](std::string nullString) {
            return unit<Null*>(new Null());
         });
}

PARSER(JSONValue*) arrayElem() {
    return token(option
            (bind<std::string, JSONValue*>
                (reserved(","),
                 [](std::string unused) {
                    return value();
                 })
            , value()));
}

PARSER(Array*) array() {
    return token(bind<std::vector<JSONValue*>, Array*>
        (squareBraces(many<JSONValue*>(arrayElem())),
         [](std::vector<JSONValue*> arr) {
            Array* newArray = new Array();
            newArray->content = arr;
            return unit<Array*>(newArray);
         }));
}

// Wrapped in lambda to prevent infinite loop
// while building resulting parser.
PARSER(JSONValue*) value() {
    return [](std::string s) {
        return token(option(option(option(option(option
          ( wrapString(jsonString())
          , wrapNumber(jsonNumber()))
          , wrapObject(object()))
          , wrapArray(array()))
          , wrapBoolean(boolean()))
          , wrapNull(null())))(s);
    };
}

PARSER(Pair) pair() {
    return token(bind<String*, Pair>
        (jsonString(),
         [](String* str) {
            return bind<std::string, Pair>
                (reserved(":"),
                 [str](std::string colon) {
                    return bind<JSONValue*, Pair>
                        (value(),
                         [str](JSONValue* val) {
                            Pair newPair;
                            newPair.name = str->theString;
                            newPair.value = val;
                            return unit<Pair>(newPair);
                         });
                 });
         }));
}

PARSER(Pair) member() {
    return token(option
            (bind<std::string, Pair>
                (reserved(","),
                 [](std::string unused) {
                    return pair();
                 })
            , pair()));
}

PARSER(JSONObject*) object() {
    return token(bind<std::vector<Pair>, JSONObject*>
        (braces(many<Pair>(member())),
         [](std::vector<Pair> resPairs) {
            JSONObject* resObject = new JSONObject();
            resObject->members = resPairs;
            return unit<JSONObject*>(resObject);
         }));
}

PARSER(JSONValue*) json() {
    return option(wrapObject(object()), wrapArray(array()));
}

int main() {
    std::cout << "Welcome to the JSON parser." << std::endl;
    std::cout << "Type in JSON stuff and this will atempt to parse and build it." << std::endl;
    std::cout << "Note: the structure is printed quite badly, but if it's printed, then it means it was parsed." << std::endl;
    std::cout << "Type q to quit." << std::endl;

    while (true) {
        std::string input;
        std::cout << "> ";
        std::getline(std::cin, input);
        if (input == "q")
            break;
        auto maybeResult = runParser(json(), input);
        if (maybeResult) {
            auto result = maybeResult.value();
            result->print();
        } else {
            std::cout << "Failed to parse " << input << std::endl;
        }
    }
    
    return 0;
}

