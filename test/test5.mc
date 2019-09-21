def fib(x)
  # TODO 3.5
  # Delete "1" and write fib here
  if x < 3 then
    1
  else
    fib(x - 1) + fib(x - 2)