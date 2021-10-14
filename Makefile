all:
	make -C CVRP 			
	make -C CVRPTW

libLKHSym.a:
	$(MAKE) -C SRC libLKHSym.a TREE_TYPE=TWO_LEVEL_TREE 

libLKHAsym.a:
	$(MAKE) -C SRC libLKHAsym.a TREE_TYPE=ONE_LEVEL_TREE

clean:
	make -C SRC clean
	make -C CVRP clean		
	make -C CVRPTW clean
