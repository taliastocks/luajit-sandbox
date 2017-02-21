io.write('hello world\n')
local i = 0
while true do
  if i % (10 * 1000 * 1000) == 0 then
    io.write(i .. '\n')
  end
  i = i + 1
end
