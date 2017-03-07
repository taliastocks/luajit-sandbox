coroutine_plus = {}

local _co = require "coroutine";
local _current_L


function coroutine_plus.new()

  local _lib = {}
  local _current_co

  function _lib.create(f)
    local co
    local L = _co.create(function (success, values)
      values = {f(unpack(values))}
      co[2] = "dead"
      return co[1], values
    end)
    co = {L, "suspended"}
    return co
  end

  function _lib.resume(co, ...)
    if co[2] ~= "suspended" then
      return false, "cannot resume " .. co[2] .. " coroutine"
    end

    local success, values, saved_co
    local L = co[1]

    if _current_co then
      _current_co[2] = "normal"
    end

    saved_co = _current_co
    _current_co = co

    co[1] = _current_L
    co[2] = "running"

    if _current_L then
      success, values = _co.yield(L, {...})
    else
      _current_L = L
      values = {...}
      success = true
      while success and _current_L do
        success, _current_L, values = _co.resume(_current_L, success, values)
      end
      if not success then
        values = {_current_L} -- Actually, this is an error message in this case.
      end
      _current_L = nil
    end

    if not success then
      co[2] = "dead"
    end

    _current_co = saved_co
    if _current_co then
      _current_co[2] = "running"
    end

    return success, unpack(values)
  end

  function _lib.running()
    return _current_co
  end

  function _lib.status(co)
    return co[2]
  end

  function _lib.wrap(f)
    error("not implemented") -- the trouble is that this needs to propagate errors
  end

  function _lib.yield(...)
    local L = _current_co[1]
    _current_co[1] = _current_L
    _current_co[2] = "suspended"
    success, values = _co.yield(L, {...})
    _current_co[2] = "running"
    return unpack(values)
  end

  return _lib

end


coroutine = coroutine_plus.new()

return coroutine_plus
