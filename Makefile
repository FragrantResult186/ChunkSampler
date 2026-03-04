CXX      := g++
CXXFLAGS := -O3 -std=c++17 -Wall -Wno-parentheses -Wno-reorder
LIB      := libchunksampler.a
OBJDIR   := obj

SRCS_CPP := abstract_random.cpp \
            noise.cpp \
            terrain.cpp \
            noise_column_sampler.cpp \
            chunk_noise_sampler.cpp \
            aquifer_sampler.cpp \
			md5.cpp
OBJS     := $(addprefix $(OBJDIR)/, $(SRCS_CPP:.cpp=.o))

.PHONY: lib clean

lib: $(LIB)

$(OBJDIR):
	@if not exist $(OBJDIR) mkdir $(OBJDIR)

$(LIB): $(OBJS)
	$(AR) rcs $@ $^

$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/%.o: | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	if exist $(OBJDIR) rmdir /s /q $(OBJDIR)
	if exist $(LIB) del /f /q $(LIB)
	if exist $(TARGET) del /f /q $(TARGET)
