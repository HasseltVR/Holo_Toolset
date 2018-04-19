SET(HPV_Creator_SRCS
	Log.cpp
	lz4.c
	lz4hc.c
	YCoCg.cpp
	YCoCgDXT.cpp
	HPVCreator.cpp
)

ADD_LIBRARY(HPV_Creator STATIC ${HPV_Creator_SRCS})