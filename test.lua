-- require("blah")
io.write('hello world\n')
local i = 0
local a = {}
while true do
  if i % (10 * 1000 * 1000) == 0 then
    io.write(i .. '\n')
  end
  if i % 100 == 0 then
    a[i] = i
  end
  i = i + 1
end
