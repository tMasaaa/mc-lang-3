def fibtail(a b c)
  if c < 2 then
    b
  else
    fibtail(a+b,a,c-1)
def fib(n)
  fibtail(1,1,n)