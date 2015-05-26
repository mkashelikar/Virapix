#ifndef _AppGlobal_
#define _AppGlobal_

#include "ProcUtil.h"
#include <string>

struct SConfig;

void SetSigProcMask();
void WaitTillReady(pid_t pid);
unsigned long GetMemOffset(const std::string& symbol, const std::string& lib);
void GetNewRootDir(std::string& newRootDir);
void ReadCfg(SConfig& config, const std::string& rootDir);

#endif
