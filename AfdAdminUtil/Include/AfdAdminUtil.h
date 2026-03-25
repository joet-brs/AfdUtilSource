/////////////////////////////////////////////////////////////////////////
// Convert.h
// Include file for convert.cpp
/////////////////////////////////////////////////////////////////////////

//define DllExport	__declspec( dllexport )

BYTE *   WINAPI ConvertToEBCDIC(BYTE*);
BYTE *  WINAPI ConvertToASCII(BYTE*);
BYTE WINAPI ConvertStrToHex(char* string);
BOOL WINAPI VbOpenFile(char* Filename);
