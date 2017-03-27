local exception_handlers = {}


local function throw(error)
  exception_handlers[#exception_handlers](error)
end


local function try_catch(try, catch)
  local handler
  handler = resumer.Resumer(function ()
    try()
    handler() -- no error
  end)
  local handler_index = #exception_handlers + 1
  exception_handlers[handler_index] = handler

  local error = handler()

  -- Clear the error handler stack so the catch block (and things it calls) can set its own error handlers.
  while #exception_handlers >= handler_index do
    exception_handlers[#exception_handlers] = nil
  end

  if error ~= nil then
    catch(error)
  end
end


try_catch(function ()
  print "1: level 1"
  throw "1: level 1 error"
  print "1: level 1 this shouldn't get printed"
end, function (error)
  print "1: level 1 catch"
  print(error)
end)


try_catch(function ()
  print "2: level 1 part 1"

  try_catch(function ()
    print "2: level 2"
    throw "2: level 2 error"
    print "2: level 2 this shouldn't get printed"
  end, function (error)
    print "2: level 2 catch"
    print(error)
  end)

  print "2: level 1 part 2"
  throw "2: level 1 error"
  print "2: level 1 this shouldn't get printed"
end, function (error)
  print "2: level 1 catch"
  print(error)
end)


try_catch(function ()
  print "3: level 1 part 1"

  try_catch(function ()
    print "3: level 2"
    throw "3: level 2 error"
    print "3: level 2 this shouldn't get printed"
  end, function (error)
    print "3: level 2 catch"
    print(error)

    throw "3: level 2 catch error"
    print "3: level 2 catch this shouldn't get printed"
  end)

  print "3: level 1 this shouldn't get printed"
end, function (error)
  print "3: level 1 catch"
  print(error)
end)


try_catch(function ()
  print "4: level 1 part 1"

  try_catch(function ()
    print "4: level 2"
  end, function (error)
    print "4: level 2 catch this shouldn't get printed"
    print(error)

    throw "4: level 2 catch error this shouldn't get printed"
  end)

  print "4: level 1 part 2"
  throw "4: level 1 error"
end, function (error)
  print "4: level 1 catch"
  print(error)
end)
