# libge makefile


CC = g++
CCFLAGS = -std=gnu++11 -O2 -Iinc -Iinc/unix
LD = g++
LDFLAGS = -lpthread

RM = rm -f

#OBJDIR = objects
#DEPDIR = deps

SRCS = \
    testmain.cpp \
    src/ge/ErrorData.cpp \
    src/ge/http/HttpServer.cpp \
    src/ge/http/HttpSession.cpp \
    src/ge/http/HttpUtil.cpp \
    src/ge/inet/INetUtil.cpp \
    src/ge/io/TextReader.cpp \
    src/ge/io/TextWriter.cpp \
    src/ge/text/String.cpp \
    src/ge/text/StringRef.cpp \
    src/ge/text/UnicodeUtil.cpp \
    src/ge/text/UnicodeUtil_avx.cpp \
    src/ge/text/UnicodeUtil_sse2.cpp \
    src/ge/thread/ThreadPool.cpp \
    src/ge/util/Bool.cpp \
    src/ge/util/Date.cpp \
    src/ge/util/Int8.cpp \
    src/ge/util/Int16.cpp \
    src/ge/util/Int32.cpp \
    src/ge/util/Int64.cpp \
    src/ge/util/UInt8.cpp \
    src/ge/util/UInt16.cpp \
    src/ge/util/UInt32.cpp \
    src/ge/util/UInt64.cpp \
    src/ge/util/UtilData.cpp \
    src/unix/ge/Error.cpp \
    src/unix/ge/System.cpp \
    src/unix/ge/aio/AioFile.cpp \
    src/unix/ge/aio/AioServer.cpp \
    src/unix/ge/aio/AioSocket.cpp \
    src/unix/ge/inet/INet.cpp \
    src/unix/ge/inet/INetAddress.cpp \
    src/unix/ge/inet/Socket.cpp \
    src/unix/ge/io/Console.cpp \
    src/unix/ge/io/FileInputStream.cpp \
    src/unix/ge/io/FileOutputStream.cpp \
    src/unix/ge/io/Process.cpp \
    src/unix/ge/io/RAFile.cpp \
    src/unix/ge/thread/Condition.cpp \
    src/unix/ge/thread/Mutex.cpp \
    src/unix/ge/thread/Once.cpp \
    src/unix/ge/thread/RWLock.cpp \
    src/unix/ge/thread/Thread.cpp \
    src/unix/ge/util/Double.cpp \
    src/unix/ge/util/Float.cpp \
    src/unix/gepriv/UnixUtil.cpp

DEPS = $(SRCS:.cpp=.d)

OBJS = $(SRCS:.cpp=.o)

# Main rule to build application
libgetest: $(DEPS) $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o libgetest

# Include rules from generated dependency files
ifneq ($(MAKECMDGOALS),clean)
	-include $(DEPS)
endif

# Rule if you need to make a .d file from a .cpp file
%.d : %.cpp
	$(CC) $(CCFLAGS) -MM -MF"$@" -MT"$(<:.cpp=.o)" "$<"

# -MM Generate dependency rules, skipping system headers
# -MF write the generated dependency to a file
# -MT Add a target to the generated dependency

# "$@" is the target (whatever.d)
# "$<" is the prerequisite (whatever.cpp)
# "$(<:.cpp=.o)" replaces the prerequisite's .cpp extension with .o

# Rule to build .o files from .cpp files
%.o : %.cpp
	$(CC) $(CCFLAGS) -c "$<" -o "$(<:.cpp=.o)"

# -c indicates that you should compile without linking
# "$<" is the prerequisite (whatever.cpp)
# -o sets the output file
# $(<:.cpp=.o) replaces the prerequisite's .cpp extension with .o

# Clean rule
.PHONY : clean
clean:
	-$(RM) $(DEPS) $(OBJS) libgetest
