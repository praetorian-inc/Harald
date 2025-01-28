# Detect OS
OS := $(shell uname -s 2>/dev/null|| echo Unknown)
# Compiler
CC = gcc
LIB_PATH_FLAG = -L
LIB_ADD_FLAG = -l
OUT_FLAG = -o
# Debug mode
ifdef DEBUG
    CFLAGS += -g -DDEBUG
else
    CFLAGS += -O2
endif

ifeq ($(OS),Linux)
	CFLAGS = -fPIC
    SHARED_EXT = so
	OBJECT_EXT = o
    SHARED_FLAGS = -shared -fPIC
    OPENSSL_CFLAGS = $(shell pkg-config --cflags openssl)
    OPENSSL_LIBS = $(shell pkg-config --libs openssl)
endif

ifeq ($(OS),Darwin)
    SHARED_EXT = dylib
	OBJECT_EXT = o
    SHARED_FLAGS = -shared -fPIC
    OPENSSL_CFLAGS = -I/usr/local/opt/openssl/include
    OPENSSL_LIBS = -L/usr/local/opt/openssl/lib -lssl -lcrypto
endif

ifeq ($(OS),Unknown)
    CC = cl
    CFLAGS = /O2 /DWIN32
    LDFLAGS = /link
    SHARED_FLAGS = /LD # Create a DLL
    SHARED_EXT = dll
	OBJECT_EXT = obj
    # OpenSSL paths (Adjust if needed)
    OPENSSL_INCLUDE = "C:\Program Files\OpenSSL-Win64\include"
    OPENSSL_LIB = "C:\Program Files\OpenSSL-Win64\lib\VC\x64\MT"
    OPENSSL_CFLAGS = /D_WIN32 /I$(OPENSSL_INCLUDE)
    OPENSSL_LIBS = /link /LIBPATH:$(OPENSSL_LIB) libssl.lib libcrypto.lib ws2_32.lib
	ifeq ($(DEBUG), 1)
			CFLAGS +=/Zi -DDEBUG
	endif
	LIB_PATH_FLAG = /LIBPATH:
	LIB_ADD_FLAG = 
	OUT_FLAG = /o
	EXEC_EXTENSION = .exe
endif



# Targets
LIB_NAME = libharald.$(SHARED_EXT)
OB_NAME = harald.$(OBJECT_EXT)
TEST1_NAME = test_harald$(EXEC_EXTENSION)
TEST2_NAME = test_harald_gh$(EXEC_EXTENSION)

# Compile the library
lib: $(OB_NAME)
	$(CC) $(SHARED_FLAGS) $(OUT_FLAG) $(LIB_NAME) $(OB_NAME) $(OPENSSL_LIBS)

# Compile both test programs
test: $(TEST1_NAME) $(TEST2_NAME)

$(TEST1_NAME): test_harald.$(OBJECT_EXT) $(LIB_NAME)
	$(CC) $(OUT_FLAG) $(TEST1_NAME) test_harald.$(OBJECT_EXT) $(LIB_PATH_FLAG). $(LIB_ADD_FLAG)harald $(OPENSSL_LIBS)

$(TEST2_NAME): test_harald_gh.$(OBJECT_EXT) $(LIB_NAME)
	$(CC) $(OUT_FLAG) $(TEST2_NAME) test_harald_gh.$(OBJECT_EXT) $(LIB_PATH_FLAG). $(LIB_ADD_FLAG)harald $(OPENSSL_LIBS)

harald.$(OBJECT_EXT): harald.c
	$(CC) $(CFLAGS) $(OPENSSL_CFLAGS) -c harald.c $(OUT_FLAG) $(OB_NAME)

test_harald.$(OBJECT_EXT): test_harald.c
	$(CC) $(CFLAGS) $(OPENSSL_CFLAGS) -c test_harald.c $(OUT_FLAG) test_harald.$(OBJECT_EXT)

test_harald_gh.$(OBJECT_EXT): test_harald_gh.c
	$(CC) $(CFLAGS) $(OPENSSL_CFLAGS) -c test_harald_gh.c $(OUT_FLAG) test_harald_gh.$(OBJECT_EXT)

# Clean up
clean:
	rm -f *$(OB_NAME) $(LIB_NAME) $(TEST1_NAME) $(TEST2_NAME)
