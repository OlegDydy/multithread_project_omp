#include "some_proc_info.h"
#define _WIN32_DCOM
#include <iostream>
#include <sstream>
#include <exception>
#include <comdef.h>
#include <Wbemidl.h>

#pragma comment(lib, "wbemuuid.lib")

processor_info get_processor_info()
{
  processor_info result;
  HRESULT hres;

  // Step 1: --------------------------------------------------
  // Initialize COM. ------------------------------------------

  hres = CoInitializeEx(0, COINIT_MULTITHREADED);
  if (FAILED(hres))
  {
    // Program has failed.
    std::stringstream ss;
    ss << "Failed to initialize COM library. Error code = 0x"
      << std::hex << hres;
    throw std::exception(ss.str().c_str());
  }

  // Step 2: --------------------------------------------------
  // Set general COM security levels --------------------------

  hres = CoInitializeSecurity(
    NULL,
    -1,                          // COM authentication
    NULL,                        // Authentication services
    NULL,                        // Reserved
    RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
    RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
    NULL,                        // Authentication info
    EOAC_NONE,                   // Additional capabilities 
    NULL                         // Reserved
  );


  if (FAILED(hres))
  {
    std::stringstream ss;
    ss << "Failed to initialize security.Error code = 0x"
      << std::hex << hres;
    CoUninitialize();
    throw std::exception(ss.str().c_str());
  }

  // Step 3: ---------------------------------------------------
  // Obtain the initial locator to WMI -------------------------

  IWbemLocator *pLoc = NULL;

  hres = CoCreateInstance(
    CLSID_WbemLocator,
    0,
    CLSCTX_INPROC_SERVER,
    IID_IWbemLocator, (LPVOID *)&pLoc);

  if (FAILED(hres))
  {
    std::stringstream ss;
    ss << "Failed to create IWbemLocator object."
      << " Err code = 0x"
      << std::hex << hres;
    CoUninitialize();
    throw std::exception(ss.str().c_str());
    // Program has failed.
  }

  // Step 4: -----------------------------------------------------
  // Connect to WMI through the IWbemLocator::ConnectServer method

  IWbemServices *pSvc = NULL;

  // Connect to the root\cimv2 namespace with
  // the current user and obtain pointer pSvc
  // to make IWbemServices calls.
  hres = pLoc->ConnectServer(
    _bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
    NULL,                    // User name. NULL = current user
    NULL,                    // User password. NULL = current
    0,                       // Locale. NULL indicates current
    NULL,                    // Security flags.
    0,                       // Authority (for example, Kerberos)
    0,                       // Context object 
    &pSvc                    // pointer to IWbemServices proxy
  );

  if (FAILED(hres))
  {
    std::stringstream ss;
    ss << "Could not connect. Error code = 0x"
      << std::hex << hres;
    pLoc->Release();
    CoUninitialize();

    throw std::exception(ss.str().c_str());
    // Program has failed.
  }

  // cout << "Connected to ROOT\\CIMV2 WMI namespace" << endl;


  // Step 5: --------------------------------------------------
  // Set security levels on the proxy -------------------------

  hres = CoSetProxyBlanket(
    pSvc,                        // Indicates the proxy to set
    RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
    RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
    NULL,                        // Server principal name 
    RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
    RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
    NULL,                        // client identity
    EOAC_NONE                    // proxy capabilities 
  );

  if (FAILED(hres))
  {
    std::stringstream ss;
    ss << "Could not set proxy blanket. Error code = 0x"
      << std::hex << hres;
    pSvc->Release();
    pLoc->Release();
    CoUninitialize();

    throw std::exception(ss.str().c_str());
    // Program has failed.
  }

  // Step 6: --------------------------------------------------
  // Use the IWbemServices pointer to make requests of WMI ----

  // For example, get the name of the operating system
  IEnumWbemClassObject* pEnumerator = NULL;
  hres = pSvc->ExecQuery(
    bstr_t("WQL"),
    bstr_t("SELECT * FROM Win32_Processor"),
    WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
    NULL,
    &pEnumerator);

  if (FAILED(hres))
  {
    std::stringstream ss;
    ss << "Query for operating system name failed."
      << " Error code = 0x"
      << std::hex << hres;
    pSvc->Release();
    pLoc->Release();
    CoUninitialize();

    throw std::exception(ss.str().c_str());
  }

  // Step 7: -------------------------------------------------
  // Get the data from the query in step 6 -------------------

  IWbemClassObject *pclsObj = NULL;
  ULONG uReturn = 0;

  while (pEnumerator)
  {
    HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
      &pclsObj, &uReturn);

    if (0 == uReturn)
    {
      break;
    }

    _variant_t vtProp;

    // Get the value of the CPU Name property
    hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
    result.name = (_bstr_t)vtProp;

    // Get the count of CPU cores
    hr = pclsObj->Get(L"NumberOfCores", 0, &vtProp, 0, 0);
    result.cores = (unsigned)vtProp;

    // Get the count of Logical Threads
    hr = pclsObj->Get(L"NumberOfLogicalProcessors", 0, &vtProp, 0, 0);
    result.threads = (unsigned)vtProp;

    // Get the frequence of CPU (MHz)
    hr = pclsObj->Get(L"MaxClockSpeed", 0, &vtProp, 0, 0);
    result.freqency = (uint32_t)vtProp;

    // Get the size of cache L1 (Kb)
    result.L1 = 32;

    // Get the size of cache L2 (Kb)
    hr = pclsObj->Get(L"L2CacheSize", 0, &vtProp, 0, 0);
    result.L2 = (unsigned)vtProp;

    // Get the size of cache L3 (Kb)
    hr = pclsObj->Get(L"L3CacheSize", 0, &vtProp, 0, 0);
    result.L3 = (unsigned)vtProp;

    pclsObj->Release();
  }

  // Cleanup
  // ========

  pSvc->Release();
  pLoc->Release();
  pEnumerator->Release();
  CoUninitialize();

  return result;
}
