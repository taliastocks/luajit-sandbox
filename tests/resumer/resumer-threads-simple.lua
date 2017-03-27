local thread1 = resumer.Resumer(function (input)
  local food = input "thread1: What would you like to eat today?"
  print("thread1: You chose to eat " .. food .. ".")
  local drink = input "thread1: And what would you like to drink?"
  print("thread1: You chose to drink " .. drink .. ".")
end)

local thread2 = resumer.Resumer(function (input)
  local answer = input "thread2: Do you like spam?"
  print("thread2: Your answer: " .. answer)
end)

print(thread1(thread1))
print(thread2(thread2))

print(thread1 "salmon")
thread2 "I have never tried spam, and I don't wish to start now."
thread1 "orange juice"
