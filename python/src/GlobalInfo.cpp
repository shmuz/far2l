#include <farplug-wide.h>

#ifdef FAR_MANAGER_FAR2M
SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  struct VersionInfo Version = { 0,0,0,0 };
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0x7E9585C2;
  aInfo->Version       = Version;
  aInfo->Title         = L"Python";
  aInfo->Description   = L"Python plugin for Far Manager";
  aInfo->Author        = L"Grzegorz Makarewicz";
}
#endif
