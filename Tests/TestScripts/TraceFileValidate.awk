#!/bin/awk -f
BEGIN{
	allocations=0
	count64=0
	count32=0
}
/^a.*/ {
size = substr($4,2)+0;
pointer64 = substr($5,2)*8+16;
pointer32 = substr($5,2)*4+8;
allocations++;
if(size < pointer64) count64++;
if(size < pointer32) count32++;}
END{
	print allocations " " count64 " " count32;
}

