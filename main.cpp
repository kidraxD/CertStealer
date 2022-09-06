#include <iostream>
#include <Windows.h>
#include <fstream>


PVOID read_file_by_name(LPCSTR file_path, int* size)
{
	HANDLE h_dll = CreateFileA(file_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h_dll == INVALID_HANDLE_VALUE)
		return NULL;

	DWORD dll_file_sz = GetFileSize(h_dll, NULL);
	*size = dll_file_sz;
	PVOID dll_buffer = VirtualAlloc(NULL, dll_file_sz, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!ReadFile(h_dll, dll_buffer, dll_file_sz, NULL, FALSE) || *(PDWORD)dll_buffer != 9460301)
	{
		VirtualFree(dll_buffer, 0, MEM_RELEASE);
		goto exit;
	}

exit:
	CloseHandle(h_dll);
	return dll_buffer;
	
}

PIMAGE_NT_HEADERS get_nt_header(PVOID base)
{
	auto dos_headers = PIMAGE_DOS_HEADER(base);
	return PIMAGE_NT_HEADERS((uintptr_t)base + dos_headers->e_lfanew);
}

int main(int argc, char** argv)
{
	int unsigned_file_size;
	int signed_file_size;
	if (argc < 2)
	{
		printf("First Argument: unsigned file | Second Argument: signed file | Third Argument: final signed file path");
		return 0;
	}
	PVOID unsigned_file = read_file_by_name(argv[1], &unsigned_file_size);
	PVOID signed_file = read_file_by_name(argv[2],&signed_file_size);

	
	PIMAGE_NT_HEADERS signed_nt_header = get_nt_header(signed_file);

	IMAGE_DATA_DIRECTORY sign_data = signed_nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];
	int sizeof_sign_data = sign_data.Size;

	BYTE* final_file = new BYTE[unsigned_file_size + sizeof_sign_data];

	memcpy(final_file, (PVOID)unsigned_file, unsigned_file_size);
	memcpy(PVOID(final_file + unsigned_file_size), (PVOID)((uintptr_t)signed_file + sign_data.VirtualAddress), sizeof_sign_data);

	PIMAGE_NT_HEADERS final_file_nt_header = get_nt_header(final_file);
	final_file_nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].VirtualAddress = unsigned_file_size;
	final_file_nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].Size = sizeof_sign_data;



	std::ofstream test(argv[3] , std::ios::binary);
	test.write((const char*)final_file, unsigned_file_size + sizeof_sign_data);
	test.close();

	printf("Done!\n");
}
