.PHONY: all  
all:  
	cd logic; $(MAKE); cd ../merge; $(MAKE);
  
.PHONY: clean  
clean:  
	cd logic; $(MAKE) clean ; cd ../merge; $(MAKE) clean ;

.PHONY: install 
install:  
	cd logic; $(MAKE) install; cd ../merge; $(MAKE) install;
