# libge makefile


CC := gcc
CCFLAGS := -O2
LDFLAGS := -lpthread

RM := rm -f


SRCS = testmain.cpp \
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
	src/unix/ge/ErrorData.cpp \
	src/unix/ge/System.cpp \
	src/unix/ge/aio/Selector.cpp \
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
	src/unix/ge/thread/Semaphore.cpp \
	src/unix/ge/thread/Thread.cpp \
	src/unix/ge/util/Double.cpp \
	src/unix/ge/util/Float.cpp \
	src/unix/gepriv/UnixUtil.cpp

DEPS = $(SRCS:.c=.d)

OBJS = $(SRCS:.c=.o)

# Main rule to build application
libgetest: $(DEPS) $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o libgetest

# Include rules from generated dependency files
-include $(DEPS)

# Rule if you need to make a .d file from a .c file
%.d : %.c
	$(CC) $(CCFLAGS) -MM -MF"$@" -MT"$(<:.c=.o)" "$<"

# -MM Generate dependency rules, skipping system headers
# -MF write the generated dependency to a file
# -MT Add a target to the generated dependency

# "$@" is the target (whatever.d)
# "$<" is the prerequisite (whatever.c)
# "$(<:.c=.o)" replaces the .c extension with .o

# Rule to build .o files from .c files
%.o : %.c
	$(CC) $(CCFLAGS) -c "$<"

# "$<" is the prerequisite (whatever.c)

# Clean rule
.PHONY : clean
clean:
	$(RM) $(DEPS) $(OBJS) libgetest
