all: 
	make -C CVRP 			
	make -C CVRPTW
	mkdir -p CVRPController/build
	cmake -S CVRPController -B CVRPController/build -DCMAKE_BUILD_TYPE=Release
	$(MAKE) -C CVRPController/build/ -j
	mkdir -p VRPTWController/build
	cmake -S VRPTWController -B VRPTWController/build -DCMAKE_BUILD_TYPE=Release
	$(MAKE) -C VRPTWController/build/ -j

	
libLKHSym.a:
	$(MAKE) -C COMMON libLKHSym.a TREE_TYPE=TWO_LEVEL_TREE 

libLKHAsym.a:
	$(MAKE) -C COMMON libLKHAsym.a TREE_TYPE=ONE_LEVEL_TREE

clean:
	make -C COMMON clean
	make -C CVRP clean		
	make -C CVRPTW clean
	$(MAKE) -C cvrpController/build/ clean
	$(MAKE) -C VRPTWController/build/ clean
