cmd_Release/strong-agent.node := ./gyp-mac-tool flock ./Release/linker.lock c++ -bundle -Wl,-search_paths_first -mmacosx-version-min=10.5 -arch x86_64 -L./Release  -o Release/strong-agent.node Release/obj.target/strong-agent/src/strong-agent.o -undefined dynamic_lookup
