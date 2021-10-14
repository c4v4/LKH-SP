all:
	make -C CVRP 			
	make -C CVRPTW

libLKHSym.a:
	$(MAKE) -C LKH-3 libLKHSym.a TREE_TYPE=TWO_LEVEL_TREE 

libLKHAsym.a:
	$(MAKE) -C LKH-3 libLKHAsym.a TREE_TYPE=ONE_LEVEL_TREE

clean:
	make -C LKH-3 clean

