local runnable = {}
local current_thread

local function Thread(entrypoint)
  local thread = {}
  thread.resumer = resumer.Resumer(function ()
    entrypoint()
    for i = 1, #thread.waiters do
      runnable[#runnable + i] = thread.waiters[i]
    end
  end)
  thread.waiters = {}
  runnable[#runnable + 1] = thread
  return thread
end


local main_thread = Thread(function ()
  print "main_thread creating threads 1-4"
  local thread_1, thread_2, thread_3, thread_4
  thread_1 = Thread(function ()
    print "thread_1 will create a new thread and wait for it"
    local thread_1_child = Thread(function ()
      print "thread_1_child just exits"
    end)
    thread_1_child.waiters[#thread_1_child.waiters + 1] = current_thread
    current_thread.resumer()
    print "thread_1 resumed, and will exit"
  end)
  thread_2 = Thread(function ()
    print "thread_2 will wait for thread_1"
    thread_1.waiters[#thread_1.waiters + 1] = current_thread
    current_thread.resumer() -- yields
    print "thread_2 resumed, and will exit"
  end)
  thread_3 = Thread(function ()
    print "thread_3 will wait for thread_4"
    thread_4.waiters[#thread_4.waiters + 1] = current_thread
    current_thread.resumer() -- yields
    print "thread_3 resumed, and will exit"
  end)
  thread_4 = Thread(function ()
    print "thread_4 will wait for thread_2"
    thread_2.waiters[#thread_2.waiters + 1] = current_thread
    current_thread.resumer() -- yields
    print "thread_4 resumed, and will exit"
  end)
  print "main_thread exiting"
end)


while #runnable > 0 do
  current_thread = runnable[#runnable]
  runnable[#runnable] = nil
  current_thread.resumer()
  current_thread = nil
end
