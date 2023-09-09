#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  struct VersionInfo Version = { 2023,9,1,0 };
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0xE873426D;
  aInfo->Version       = Version;
  aInfo->Title         = L"NetCfg";
  aInfo->Description   = L"NetCfg plugin for Far Manager";
  aInfo->Author        = L"VPROFi";
}
