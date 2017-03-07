require "coroutine_plus"

generator = coroutine_plus.new()

function count(n)
  while true do
    print("counting to " .. n)
    for i = 1, n do
      generator.yield(i) -- yields to hello in this demo
    end
    n = coroutine.yield() -- get the next number to count to
  end
end

function hello(n)
  local gen = generator.create(count)
  while true do
    success, k = generator.resume(gen, n)
    if not success then
      break
    end
    print("hello", k)
  end
end

hello = coroutine.create(hello)

coroutine.resume(hello, 3)
coroutine.resume(hello, 5)
coroutine.resume(hello, 2)
