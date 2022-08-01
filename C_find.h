extern unsigned int *TCStreamClock_TCStreamClock;
extern unsigned int *TCStreamClock_destructor;
extern unsigned int *TCStreamClock_Create;
extern unsigned int *TCStreamClock_Destroy;

void *C_sub_find(void *h, const char *fn_name)
{
	char fname[256];
	void **ret;
	unsigned int *addr;

	C_CASE(_ZN18TCRRTItemDimensionD2Ev)
		addr = find_nth_func_from_export(h, "_ZN20IDescriptorContainerD1Ev", 16);
		C_FOUND(addr);
	C_RET(addr);

	C_CASE(_ZN13TCStreamClock11StreamClockEPb)
		addr = find_nth_func_by_string(h, "_ZN20IDescriptorContainerD1Ev", "StreamClock", F_SEEK_UP, 0x0L, 0);
		C_FOUND(addr);
	C_RET(addr);

	C_CASE(_ZN13TCStreamClock14SetStreamClockEmi)
		addr = find_nth_func_by_string(h, "_ZN20IDescriptorContainerD1Ev", "SetStreamClock", F_SEEK_UP, 0x0L, 0);
		C_FOUND(addr);
	C_RET(addr);

	C_CASE(_ZN4TCTv6SetDSTEi)
		addr = find_nth_func_by_string(h, "_ZN4TCTv6SetDSTEi", "[TvBase::SetDST]", F_SEEK_UP, -0x500L, 0);
		C_FOUND(addr);
	C_RET(addr);

	C_CASE(_ZN13TCStreamClockC1Ev)
		addr = TCStreamClock_TCStreamClock;
		if(addr) C_FOUND(addr);
	C_RET(addr);

	C_CASE(_ZN13TCStreamClockD1Ev)
		addr = TCStreamClock_destructor;
		if(addr) C_FOUND(addr);
	C_RET(addr);

	C_CASE(_ZN13TCStreamClock6CreateEi)
		addr = TCStreamClock_Create;
		if(addr) C_FOUND(addr);
	C_RET(addr);

	C_CASE(_ZN13TCStreamClock7DestroyEv)
		addr = TCStreamClock_Destroy;
		if(addr) C_FOUND(addr);
	C_RET(addr);

	return 0;
}
