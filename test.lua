local foo = resumer.Resumer(function (yield, ...)
  for i = 1, 4 do
    yield(i)
  end
  return #{...}, ...
end)

print(foo(foo, 1, 2, 3))
print(foo())
print(foo())
print(foo())
print(foo())
print(foo("hello!"))
