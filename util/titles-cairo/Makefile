TARGET = titles-cairo
OBJS   = cairo1.o

INCLUDES += -I/opt//local/include/
INCLUDES += -I/opt//local/include/freetype2/
INCLUDES += -I/opt/local/include/cairo/
INCLUDES += -I/opt/local/include/cairomm-1.0/ 
INCLUDES += -I/opt/local/lib/cairomm-1.0/include/ 
INCLUDES += -I/opt/local/include/sigc++-2.0/
INCLUDES += -I/opt/local/lib/sigc++-2.0/include/

CXXFLAGS = -std=c++11 $(INCLUDES)
#LDFLAGS  = -lglfw3 -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
LDFLAGS = -L/opt/local/lib -lcairo -lcairomm-1.0 -lsigc-2.0

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

