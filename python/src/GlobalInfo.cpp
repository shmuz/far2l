#include <farplug-wide.h>

SHAREDSYMBOL void WINAPI EXP_NAME(GetGlobalInfo)(struct GlobalInfo *aInfo)
{
  aInfo->StructSize    = sizeof(*aInfo);
  aInfo->SysID         = 0x7E9585C2;
  aInfo->Version       = MAKEPLUGVERSION(0,0,0,0);
  aInfo->Title         = L"Python";
  aInfo->Description   = L"Python plugin for Far Manager";
  aInfo->Author        = L"Grzegorz Makarewicz";
}
