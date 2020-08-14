#!/bin/bash

assert () {
  expected="$1"
  input="$2"

  ./dcc "$input" > tmp.s
  cc -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert_link () {
  expected="$1"
  input="$2"
  linked="$3"

  ./dcc "$input" > tmp.s
  cc -o tmp tmp.s "$linked"
  ./tmp
}

#assert 0 0
#assert 42 42

#assert 21 '5+20-4'
#assert 41 " 12 + 34 - 5 "

#assert 47 "5+6*7"
#assert 15 '5*(9-6)'
#assert 4  '(3+5)/2'

#assert 10 '-10+20'

#assert 0 '0==1'
#assert 1 '42==42'
#assert 1 '0!=1'
#assert 0 '42!=42'

#assert 1 '0<1'
#assert 0 '1<1'
#assert 0 '2<1'
#assert 1 '0<=1'
#assert 1 '1<=1'
#assert 0 '2<=1'

#assert 1 '1>0'
#assert 0 '1>1'
#assert 0 '1>2'
#assert 1 '1>=0'
#assert 1 '1>=1'
#assert 0 '1>=2'

assert 3 "a = 3;"
assert 22 "b = 5 * 6 - 8;"
assert 14 " a = 3; b = 5 * 6 - 8; a + b / 2;"

assert 6 "foo=1; bar=2+3; foo + bar;"

assert 6 "foo=1; bar=2+3; return foo+bar;"

assert 1 "if (1) return 1; else return 2;"
assert 5 "x=0; while(x<5) x=x+1;"
assert 10 "x=0; for (i=0; i<5; i=i+1) x=x+i; x=x;"

assert 10 "x=0; for (i=0; i<5; i=i+1) {x=x+i;} return x;"

assert_link 0 "foo (1,2,3,4);" foo.o

echo OK
