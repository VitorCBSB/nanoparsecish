#include "nanoparsecish.h"
#include <iostream>

// Arithmetic expression parser. Covers addition, subtraction and multiplication.
// Adding division is trivial though.

// Pay no mind to the egregious memory leaks. I'm too lazy to do this right for a toy thing.

struct Expr {
    virtual int eval() = 0;
};

struct Add : public Expr {
    Expr* op1;
    Expr* op2;
    
    Add(Expr* opA, Expr* opB) : op1(opA), op2(opB) {
    }
    
    int eval() {
        return op1->eval() + op2->eval();
    }
};

struct Minus : public Expr {
    Expr* op1;
    Expr* op2;
    
    Minus(Expr* opA, Expr* opB) : op1(opA), op2(opB) {
    }
    
    int eval() {
        return op1->eval() - op2->eval();
    }
};

struct Mult : public Expr {
    Expr* op1;
    Expr* op2;
    
    Mult(Expr* opA, Expr* opB) : op1(opA), op2(opB) {
    }
    
    int eval() {
        return op1->eval() * op2->eval();
    }
};

struct Lit : public Expr {
    int literal;
    
    int eval() {
        return literal;
    }
};

Expr* makeAdd(Expr* op1, Expr* op2) {
    return new Add(op1, op2);
}

Expr* makeMinus(Expr* op1, Expr* op2) {
    return new Minus(op1, op2);
}

Expr* makeMult(Expr* op1, Expr* op2) {
    return new Mult(op1, op2);
}

template <typename T>
PARSER(std::function<T(T, T)>) infixOp(std::string opStr, std::function<T(T, T)> f) {
    return bind<std::string, std::function<T(T, T)>>
        (reserved(opStr),
            [f](std::string op) {
                return unit<std::function<T(T, T)>>(f);
            });
}

PARSER(std::function<Expr*(Expr*, Expr*)>) addOp() {
    return option(infixOp<Expr*>("+", makeAdd), infixOp<Expr*>("-", makeMinus));
}

PARSER(std::function<Expr*(Expr*, Expr*)>) mulOp() {
    return infixOp<Expr*>("*", makeMult);
}

PARSER(Expr*) expr();
PARSER(Expr*) term();
PARSER(Expr*) factor();
PARSER(Expr*) literal();

PARSER(Expr*) expr() {
    return chainl1(term(), addOp());
}

PARSER(Expr*) term() {
    return chainl1(factor(), mulOp());
}

/* So, turns out you can't build "infinitely" recursive parsers like this.
    C++ will attempt to evaluate both arguments before calling option(),
    which I guess is reasonable, but it fucks my plan up as it results 
    in an infinite loop, so we have to wrap it in a lambda in order to 
    make it NOT evaluate the parsers while building them at some point.
    I chose the factor function, but it'd probably be better to choose 
    the top-level function of the loop.
    
PARSER(Expr*) factor() {
    return option(literal(), parens(expr()));
}
*/

PARSER(Expr*) factor() {
    return [](std::string s) {
        return option(literal(), parens(expr()))(s);
    };
}

PARSER(Expr*) literal() {
    return bind<int, Expr*>
        (number(), 
                [](int val) {
                    Lit* literal = new Lit();
                    literal->literal = val;
                    return unit<Expr*>(literal);
                });
}

int main() {
    std::cout << "Welcome to the expression parser." << std::endl;
    std::cout << "Type in arithmetic expressions the way you're used to." << std::endl;
    std::cout << "Type q to quit." << std::endl;

    while (true) {
        std::string input;
        std::cout << "> ";
        std::getline(std::cin, input);
        if (input == "q")
            break;
        auto maybeResult = runParser(expr(), input);
        if (maybeResult) {
            auto result = maybeResult.value();
            std::cout << result->eval() << std::endl;
        } else {
            std::cout << "Failed to parse " << input << std::endl;
        }
    }
    
    return 0;
}

