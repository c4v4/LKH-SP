all:
	make -C CVRP 			
	make -C CVRPTW

libLKHSym.a:
	$(MAKE) -C COMMON libLKHSym.a TREE_TYPE=TWO_LEVEL_TREE 

libLKHAsym.a:
	$(MAKE) -C COMMON libLKHAsym.a TREE_TYPE=ONE_LEVEL_TREE

clean:
	make -C COMMON clean
	make -C CVRP clean		
	make -C CVRPTW clean
