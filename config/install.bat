@if not exist %2\nul mkdir %2
@rm -f %2\%1
@cp %1 %2
