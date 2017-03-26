--! luajit-sandbox -m 100
io.stdout:setvbuf "no"
local a = {}
while true do
  a[#a + 1] = 0
  if #a % 1000000 == 0 then
    print(#a)
  end
end
