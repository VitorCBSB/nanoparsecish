# NanoParsec... ish.

This is a mostly experimental project in porting parser combinators in the style of Parsec to C++.

I mostly followed Stephen Diehl's Write You A Haskell book. More specifically, [Chapter 3](http://dev.stephendiehl.com/fun/002_parsers.html).

It's kind of ugly, to be honest, due to mostly missing a ton of sugar from Haskell (I miss you infix operators and type inference), but I find it to work surprisingly well.

This parser has absolutely no focus on efficiency, as might be observed by the countless copies everywhere. This was not a goal of this experiment.

Anyway, there are two examples at the moment. A parser for arithmetic expressions and a parser for [JSON](http://json.org). Both contain an interactive loop which continuosly feeds the parser with input from the keyboard. To compile them:

```
g++ -std=c++1y json.cpp -o json
g++ -std=c++1y arith.cpp -o arith
```

Yes, I'm too lazy to create a Makefile (I kinda dislike them).

Anyways, enjoy.

