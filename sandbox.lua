-- This is just an example of using rlimit from luajit. It doesn't actually provide a sandbox of any kind.

local ffi = require "ffi"
local C = ffi.C

ffi.cdef[[
typedef struct rlimit {
  uint64_t rlim_cur;
  uint64_t rlim_max;
} rlimit_t;

int fork(void);
int setrlimit(int, const struct rlimit *);

int waitpid(int, int *, int);
]]

local rlimit_t = ffi.typeof("rlimit_t")

local pid = C.fork() -- called once, returns twice

if pid == 0 then -- this is the child process

  if C.setrlimit(0, rlimit_t(1, 1)) ~= 0 then -- CPU limit
    io.write("failed to set CPU limit\n")
    os.exit(1)
  end

  local i = 0
  while true do
    if i % 10000000 == 0 then
      io.write("still alive ", i, "\n")
    end
    i = i + 1
  end
end


local status = ffi.new("int[1]") -- there are no pointers, so use an array
C.waitpid(pid, status, 0)


io.write("child exited with status code ", status[0], "\n")

io.write "goodbye\n"
