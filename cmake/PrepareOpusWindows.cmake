include(FetchContent)
FetchContent_Declare(opus
  GIT_REPOSITORY https://github.com/xiph/opus.git
  GIT_TAG        61747bc6ec728de69d54db6ece90ad4617f059b8 # master at the time of adding this
)

FetchContent_MakeAvailable(opus)
FetchContent_GetProperties(opus
						   SOURCE_DIR OPUS_SRC
						   BINARY_DIR OPUS_BIN
						   POPULATED  OPUS_POP)

# Windows is the outlier which we want to match macOS and Linux. When we get opus via FetchContent,
# the include file directory does not have an `opus` folder, with the includes inside that folder.
# There is only an include folder, with opus includes inside it. We want to create a opus directory
# inside that folder, and copy all the files to be inside there. That way, the source includes match up.
if(NOT EXISTS ${OPUS_SRC}/include/opus)
	file(MAKE_DIRECTORY ${OPUS_SRC}/include/opus)		# Make the directory we want to use for includes
	file(GLOB opus_includes ${OPUS_SRC}/include/*.h) 	# Get a list of files we want to copy

	# Make a copy of each header in the new directory
	foreach(path ${opus_includes})
		file(COPY ${path} DESTINATION ${OPUS_SRC}/include/opus/)
	endforeach()

endif()
target_include_directories(opus PRIVATE ${OPUS_SRC}/include/)
set(Opus_FOUND TRUE) # Set manually since we know we have it, and the find_package command won't run