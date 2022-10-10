// ExtractChipInfo.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "ExtractChipInfo.h"
#include <stdint.h>

/*
** The devicedb.h was extracted from "DLL430_v3\src\TI\DLL430\DeviceDb\devicedb.h" from MSPDebugStack_OS_Package_3_15_1_1.zip
** MSPDS distribution. This is a hex image of a ZIP file containing a set of XML files with MSP430 chip data
*/
#include "devicedb.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// The one and only application object

CWinApp theApp;

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	HMODULE hModule = ::GetModuleHandle(NULL);

	if (hModule != NULL)
	{
		// initialize MFC and print and error on failure
		if (!AfxWinInit(hModule, NULL, ::GetCommandLine(), 0))
		{
			// TODO: change error code to suit your needs
			_tprintf(_T("Fatal Error: MFC initialization failed\n"));
			nRetCode = 1;
		}
		else
		{
			try
			{
				/*
				** Compile parses and stores the zip file into TI::DLL430::DeviceDb::g_database.
				** Just Write it to a file, to extract desired information.
				*/
				CFile file(_T("MSP430-devices.zip"), CFile::modeCreate | CFile::modeReadWrite);
				// Array Symbol got from TI include file. Should not change with newer versions
				file.Write(TI::DLL430::DeviceDb::g_database, sizeof(TI::DLL430::DeviceDb::g_database));
			}
			catch (CException* e)
			{
				CString msg;
				e->GetErrorMessage(CStrBuf(msg, 500), 500);
				CStringA oem(msg);
				oem.AnsiToOem();
				fputs(oem.GetString(), stderr);
			}
		}
	}
	else
	{
		// TODO: change error code to suit your needs
		_tprintf(_T("Fatal Error: GetModuleHandle failed\n"));
		nRetCode = 1;
	}

	return nRetCode;
}
